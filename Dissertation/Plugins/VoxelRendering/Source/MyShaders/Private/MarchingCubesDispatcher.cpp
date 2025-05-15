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
		SHADER_PARAMETER(FVector3f, leafPosition)
		SHADER_PARAMETER(uint32, leafDepth)		
		SHADER_PARAMETER(uint32, nodeIndex)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, isoValues)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, marchLookUp)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FVector>, outVertices)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FVector>, outNormals)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<int>, outTris)
		SHADER_PARAMETER(uint32, voxelsPerAxis)
		SHADER_PARAMETER(float, baseDepthScale)
		SHADER_PARAMETER(float, isoLevel)
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

void AddOctreeMarchingPass(FRDGBuilder& GraphBuilder, OctreeNode* node, uint32 depth, uint32* nodeIndex, FMarchingCubesDispatchParams& Params, FRDGBufferUAVRef OutTrisUAV, FRDGBufferUAVRef OutNormalsUAV, FRDGBufferUAVRef OutVerticiesUAV, FRDGBufferSRVRef InLookUpSRV) {

	if (!node) return;
	if (!(node->isLeaf)) {	
		depth++;
		for (OctreeNode* child : node->children)
			AddOctreeMarchingPass(GraphBuilder, child, depth, nodeIndex, Params, OutTrisUAV, OutNormalsUAV, OutVerticiesUAV, InLookUpSRV);
		return;
	}

	const FRDGBufferRef isoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"), sizeof(float), isoCount, node->isoAveragedValues, isoCount * sizeof(float));
	FMarchingCubes::FParameters* PassParams = GraphBuilder.AllocParameters<FMarchingCubes::FParameters>();
	PassParams->leafPosition = node->bounds.Center();
	PassParams->leafDepth = depth;
	PassParams->nodeIndex = (*nodeIndex);
	PassParams->isoValues = GraphBuilder.CreateSRV(isoValuesBuffer);
	PassParams->marchLookUp = InLookUpSRV;
	PassParams->outVertices = OutVerticiesUAV;
	PassParams->outNormals = OutNormalsUAV;
	PassParams->outTris = OutTrisUAV;
	PassParams->voxelsPerAxis = voxelsPerAxis;
	PassParams->baseDepthScale = Params.Input.baseDepthScale;
	PassParams->isoLevel = Params.Input.isoLevel;

	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FMarchingCubes> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(voxelsPerAxis, voxelsPerAxis, voxelsPerAxis), FIntVector(1, 1, 1));

	GraphBuilder.AddPass(RDG_EVENT_NAME("Marching Cubes"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount); }
	);

	(*nodeIndex)++;
}

void FMarchingCubesInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FMarchingCubesDispatchParams Params, TFunction<void(FMarchingCubesOutput OutputVal)> AsyncCallback) {
	if (Params.Input.leafCount == 0) return;

	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_MarchingCubes_Execute);
		DECLARE_GPU_STAT(MarchingCubes)
		RDG_EVENT_SCOPE(GraphBuilder, "MarchingCubes");
		RDG_GPU_STAT_SCOPE(GraphBuilder, MarchingCubes);

		typename FMarchingCubes::FPermutationDomain PermutationVector;
		TShaderMapRef<FMarchingCubes> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			const int maxVoxelCount = nodeVoxelCount * Params.Input.leafCount;
			const int vertexCount = maxVoxelCount * 15;
			const int triCount = maxVoxelCount * 15; 

			TArray<FVector3f> OutVertices;
			TArray<FVector3f> OutNormals;
			TArray<int32> OutTris;

			OutVertices.Init(FVector3f(-1, -1, -1), vertexCount);
			OutNormals.Init(FVector3f(-1, -1, -1), vertexCount);
			OutTris.Init(-1, triCount);

			FRDGBufferRef isoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"), sizeof(int), 4096, marchLookUp, 4096 * sizeof(int));
			FRDGBufferRef OutVerticesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("OutVertices_StructuredBuffer"), sizeof(FVector3f), vertexCount, OutVertices.GetData(), vertexCount * sizeof(FVector3f));
			FRDGBufferRef OutNormalsBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("OutNormals_StructuredBuffer"), sizeof(FVector3f), vertexCount, OutNormals.GetData(), vertexCount * sizeof(FVector3f));
			FRDGBufferRef OutTrisBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("OutTris_StructuredBuffer"), sizeof(int32), triCount, OutTris.GetData(), triCount * sizeof(int32));

			FRDGBufferSRVRef InLookUpSRV = GraphBuilder.CreateSRV(isoValuesBuffer);
			FRDGBufferUAVRef OutTrisUAV = GraphBuilder.CreateUAV(OutTrisBuffer);
			FRDGBufferUAVRef OutNormalsUAV = GraphBuilder.CreateUAV(OutNormalsBuffer);
			FRDGBufferUAVRef OutVerticiesUAV = GraphBuilder.CreateUAV(OutVerticesBuffer);

			uint32 nodeIndex = 0;
			AddOctreeMarchingPass(GraphBuilder, Params.Input.tree, 0, &nodeIndex, Params, OutTrisUAV, OutNormalsUAV, OutVerticiesUAV, InLookUpSRV);

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
					OutVal.outTris.Append((int32*)IBuf, triCount);
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