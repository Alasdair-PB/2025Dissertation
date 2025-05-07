#include "MarchingCubesDispatcher.h"
#include "MyShaders.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "MyShaders/Public/MarchingCubesDispatcher.h"
#include "PixelShaderUtils.h"
#include "Runtime/RenderCore/Public/RenderGraphUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MaterialShader.h"

DECLARE_STATS_GROUP(TEXT("MarchingCubes"), STATGROUP_MarchingCubes, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("MarchingCubes Execute"), STAT_MarchingCubes_Execute, STATGROUP_MarchingCubes);

class FMarchingCubes : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMarchingCubes);
	SHADER_USE_PARAMETER_STRUCT(FMarchingCubes, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, isoValues)
		SHADER_PARAMETER(FVector3f, leafPosition)
		SHADER_PARAMETER(uint32, leafDepth)

		SHADER_PARAMETER(uint32, voxelsPerAxis)
		SHADER_PARAMETER(uint32, nodeIndex)
		SHADER_PARAMETER(float, baseDepthScale)
		SHADER_PARAMETER(float, isoLevel)

		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FVector>, outVertices)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<int>, outTris)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FVector>, outNormals)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), voxelsPerAxis);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), voxelsPerAxis);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), voxelsPerAxis);
	}
};

IMPLEMENT_GLOBAL_SHADER(FMarchingCubes, "/MyShadersShaders/MarchingCubes.usf", "MarchingCubes", SF_Compute);

void FMarchingCubesInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FMarchingCubesDispatchParams Params, TFunction<void(FMarchingCubesOutput OutputVal)> AsyncCallback) {
	if (Params.Input.leafCount == 0) return;

	FRDGBuilder GraphBuilder(RHICmdList);
	{
		//SCOPE_CYCLE_COUNTER(STAT_MarchingCubes_Execute);
		//DECLARE_GPU_STAT(MarchingCubes)
		//RDG_EVENT_SCOPE(GraphBuilder, "MarchingCubes");
		//RDG_GPU_STAT_SCOPE(GraphBuilder, MarchingCubes);

		typename FMarchingCubes::FPermutationDomain PermutationVector;
		TShaderMapRef<FMarchingCubes> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			const int maxVoxelCount = nodeVoxelCount * Params.Input.leafCount;
			const int vertexCount = maxVoxelCount * 15;
			const int triCount = maxVoxelCount * 15; 

			TArray<FVector3f> OutVertices;
			TArray<FVector3f> OutNormals;
			OutVertices.Init(FVector3f(-1, -1, -1), vertexCount);
			OutNormals.Init(FVector3f(-1, -1, -1), vertexCount);

			FRDGBufferRef OutVerticesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("OutVertices_StructuredBuffer"), sizeof(FVector3f), vertexCount, OutVertices.GetData(), vertexCount * sizeof(FVector3f));
			FRDGBufferRef OutNormalsBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("OutVertices_StructuredBuffer"), sizeof(FVector3f), vertexCount, OutNormals.GetData(), vertexCount * sizeof(FVector3f));
			FRDGBufferRef OutTrisBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(int32), triCount),TEXT("OutTris_StructuredBuffer"));

			FRDGBufferUAVRef OutTrisUAV = GraphBuilder.CreateUAV(OutTrisBuffer, PF_R32_SINT);
			FRDGBufferUAVRef OutNormalsUAV = GraphBuilder.CreateUAV(OutNormalsBuffer);
			FRDGBufferUAVRef OutVerticiesUAV = GraphBuilder.CreateUAV(OutVerticesBuffer);

			uint32 nodeIndex = 0;
			auto MarchingCubes = [&](OctreeNode* node, uint32 depth, uint32* nodeIndexPtr) -> void {
				auto NodeMarch = [&](const auto& self, OctreeNode* node, uint32 depth, uint32* nodeIndex) -> void {
					
					if (!node) return;
					if (!(node->isLeaf)) {
						for (OctreeNode* child : node->children)
							self(self, child, depth++, nodeIndex);
						return;
					}
					
					const FRDGBufferRef isoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"), sizeof(float), isoCount, node->isoValues, isoCount * sizeof(float));
					FMarchingCubes::FParameters* PassParams = GraphBuilder.AllocParameters<FMarchingCubes::FParameters>();

					PassParams->voxelsPerAxis = voxelsPerAxis;
					PassParams->nodeIndex = (*nodeIndexPtr);
					PassParams->leafPosition = node->bounds.Center();
					PassParams->leafDepth = depth;
					PassParams->isoValues = GraphBuilder.CreateSRV(isoValuesBuffer);
					PassParams->baseDepthScale = Params.Input.baseDepthScale;
					PassParams->isoLevel = Params.Input.isoLevel;
					PassParams->outTris = OutTrisUAV;
					PassParams->outNormals = OutNormalsUAV;
					PassParams->outVertices = OutVerticiesUAV;

					UE_LOG(LogTemp, Warning, TEXT("Node index: %d"), PassParams->nodeIndex);
					const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
					const TShaderMapRef<FMarchingCubes> ComputeShader(ShaderMap);

					auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(voxelsPerAxis, voxelsPerAxis, voxelsPerAxis), FIntVector(1, 1, 1));

					GraphBuilder.AddPass(RDG_EVENT_NAME("Marching Cubes"), PassParams, ERDGPassFlags::AsyncCompute,
						[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
							FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount); }
					);
					(*nodeIndexPtr)++;
				};
				NodeMarch(NodeMarch, node, depth, nodeIndexPtr);
			};

			MarchingCubes(Params.Input.tree, 0, &nodeIndex);
			FRHIGPUBufferReadback* VerticesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesVertices"));
			FRHIGPUBufferReadback* TrianglesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesIndices"));
			FRHIGPUBufferReadback* NormalsReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesNormals"));

			AddEnqueueCopyPass(GraphBuilder, NormalsReadback, OutNormalsBuffer, 0u);
			AddEnqueueCopyPass(GraphBuilder, VerticesReadback, OutVerticesBuffer, 0u);
			AddEnqueueCopyPass(GraphBuilder, TrianglesReadback, OutTrisBuffer, 0u);

			auto RunnerFunc = [VerticesReadback, TrianglesReadback, NormalsReadback, AsyncCallback, vertexCount, triCount](auto&& RunnerFunc) ->
				void {
				if (VerticesReadback->IsReady() && TrianglesReadback->IsReady() && NormalsReadback->IsReady()) {
					FMarchingCubesOutput OutVal;

					void* VBuf = VerticesReadback->Lock(0);
					OutVal.outVertices.Append((FVector3f*)VBuf, vertexCount);
					VerticesReadback->Unlock();

					void* IBuf = TrianglesReadback->Lock(0);
					OutVal.outTris.Append((int*)IBuf, triCount);
					TrianglesReadback->Unlock();

					void* NBuf = NormalsReadback->Lock(0);
					OutVal.outNormals.Append((FVector3f*)NBuf, vertexCount);
					NormalsReadback->Unlock();

					AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutVal]() {AsyncCallback(OutVal); });

					delete VerticesReadback;
					delete TrianglesReadback;
					delete NormalsReadback;
				}
				else {
					AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
						RunnerFunc(RunnerFunc); });
				}
				};
			AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
				RunnerFunc(RunnerFunc); });
		}
		else {} // We silently exit here as we don't want to crash the game if the shader is not found or has an error.

	}
	GraphBuilder.Execute();
}