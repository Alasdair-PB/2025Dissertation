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

BEGIN_SHADER_PARAMETER_STRUCT(FExtractBufferParameters, )
	RDG_BUFFER_ACCESS(Source, ERHIAccess::CopySrc)
END_SHADER_PARAMETER_STRUCT()

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
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<int>, outTris)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FVector>, outNormals)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), voxelsPerAxis - 1);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), voxelsPerAxis - 1);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), voxelsPerAxis - 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FMarchingCubes, "/MyShadersShaders/MarchingCubes.usf", "MarchingCubes", SF_Compute);
void AddMarchingCubesGraphPassFromOctree(FRDGBuilder& GraphBuilder, FRDGBufferRef outVertices, FRDGBufferRef outNormals, FRDGBufferRef outTris,
	FMarchingCubesDispatchParams Params, OctreeNode* node, uint32 depth, uint32* nodeIndex) {

	if (!node) return;
	if (!(node->isLeaf)) {
		for (OctreeNode* child : node->children)
			AddMarchingCubesGraphPassFromOctree(GraphBuilder, outVertices, outNormals, outTris, Params, child, depth++, nodeIndex);
		return;
	}

	const FRDGBufferRef isoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"),
		sizeof(float), isoCount, node->isoValues, isoCount * sizeof(float));

	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FMarchingCubes> ComputeShader(ShaderMap);

	FMarchingCubes::FParameters* PassParameters = GraphBuilder.AllocParameters<FMarchingCubes::FParameters>();
	const FRDGBufferUAVRef OutVerticesUAV = GraphBuilder.CreateUAV(outVertices);
	const FRDGBufferUAVRef OutNormalsUAV = GraphBuilder.CreateUAV(outNormals);
	const FRDGBufferUAVRef OutTrisUAV = GraphBuilder.CreateUAV(outTris);

	PassParameters->isoValues = GraphBuilder.CreateSRV(isoValuesBuffer);
	PassParameters->leafDepth = depth;
	PassParameters->leafPosition = node->bounds.Center();
	PassParameters->voxelsPerAxis = Params.Input.voxelsPerAxis;
	PassParameters->nodeIndex = (*nodeIndex);
	PassParameters->baseDepthScale = Params.Input.baseDepthScale;
	PassParameters->isoLevel = Params.Input.isoLevel;
	PassParameters->outVertices = OutVerticesUAV;
	PassParameters->outNormals = OutNormalsUAV;
	PassParameters->outTris = OutTrisUAV;
	check(PassParameters->outTris);

	UE_LOG(LogTemp, Warning, TEXT("This is a debug message with value: %d"), PassParameters->nodeIndex);


	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(voxelsPerAxis - 1, voxelsPerAxis - 1, voxelsPerAxis - 1), FIntVector(8, 8, 8));
	GraphBuilder.AddPass(RDG_EVENT_NAME("Marching Cubes"), PassParameters, ERDGPassFlags::AsyncCompute,
		[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount); }
	);

	(*nodeIndex)++;
}

void FMarchingCubesInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FMarchingCubesDispatchParams Params, TFunction<void(FMarchingCubesOutput OutputVal)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		if (Params.Input.leafCount == 0) return;

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
			const int triCount = maxVoxelCount * 5 * 3;

			TArray<FVector3f> OutVertices;
			TArray<int32> OutTris;
			TArray<FVector3f> OutNormals;
			OutVertices.Init(FVector3f(-1, -1, -1), vertexCount);
			OutTris.Init(-1, triCount);
			OutNormals.Init(FVector3f(-1, -1, -1), vertexCount);

			const FRDGBufferRef OutVerticesBuffer = CreateStructuredBuffer(GraphBuilder,TEXT("OutVertices_StructuredBuffer"), sizeof(FVector3f), vertexCount, OutVertices.GetData(), vertexCount * sizeof(FVector3f));
			const FRDGBufferRef OutNormalsBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("OutVertices_StructuredBuffer"), sizeof(FVector3f), vertexCount, OutNormals.GetData(), vertexCount * sizeof(FVector3f));
			const FRDGBufferRef OutTrisBuffer = CreateStructuredBuffer(GraphBuilder,TEXT("OutTris_StructuredBuffer"), sizeof(int32), triCount, OutTris.GetData(), triCount * sizeof(int32));

			uint32 nodeIndex = 0; 
			AddMarchingCubesGraphPassFromOctree(GraphBuilder, OutVerticesBuffer, OutNormalsBuffer, OutTrisBuffer, Params, Params.Input.tree, 0, &nodeIndex);

			FRHIGPUBufferReadback* VerticesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesVertices"));
			AddEnqueueCopyPass(GraphBuilder, VerticesReadback, OutVerticesBuffer, 0u);

			FRHIGPUBufferReadback* TrianglesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesIndices"));
			AddEnqueueCopyPass(GraphBuilder, TrianglesReadback, OutTrisBuffer, 0u);

			FRHIGPUBufferReadback* NormalsReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesNormals"));
			AddEnqueueCopyPass(GraphBuilder, NormalsReadback, OutNormalsBuffer, 0u);

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
		} else {} // We silently exit here as we don't want to crash the game if the shader is not found or has an error.
	}

	GraphBuilder.Execute();
}