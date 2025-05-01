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

		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<FVector>, outVertices)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<int>, outTris)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<FVector>, outNormals)
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
void AddMarchingCubesGraphPass(FRDGBuilder& GraphBuilder, FMarchingCubes::FParameters* PassParameters, const FMarchingCubesDispatchParams Params) {
	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FMarchingCubes> ComputeShader(ShaderMap);

	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(voxelsPerAxis - 1, voxelsPerAxis - 1, voxelsPerAxis - 1), FComputeShaderUtils::kGolden2DGroupSize);
	GraphBuilder.AddPass(RDG_EVENT_NAME("Marching Cubes"), PassParameters, ERDGPassFlags::AsyncCompute,
		[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount); }
	);
}

void AddMarchingCubesGraphPassFromOctree(FRDGBuilder& GraphBuilder, FMarchingCubesDispatchParams Params, FMarchingCubes::FParameters* PassParameters
	, OctreeNode* node, uint32 depth, uint32* nodeIndex) {

	if (!node) return;
	if (!(node->isLeaf)) {
		for (OctreeNode* child : node->children)
			AddMarchingCubesGraphPassFromOctree(GraphBuilder, Params, PassParameters, child, depth++, nodeIndex);
		return;
	}

	const FRDGBufferRef isoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"),
		sizeof(float), isoCount, node->isoValues, isoCount * sizeof(float));

	PassParameters->isoValues = GraphBuilder.CreateSRV(isoValuesBuffer);
	PassParameters->leafDepth = depth;
	PassParameters->leafPosition = node->bounds.Center();
	PassParameters->voxelsPerAxis = Params.Input.voxelsPerAxis;
	PassParameters->nodeIndex = *nodeIndex;
	PassParameters->baseDepthScale = Params.Input.baseDepthScale;
	PassParameters->isoLevel = Params.Input.isoLevel;

	(*nodeIndex)++;
	AddMarchingCubesGraphPass(GraphBuilder, PassParameters, Params);
}

void FMarchingCubesInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FMarchingCubesDispatchParams Params, TFunction<void(FMarchingCubesOutput OutputVal)> AsyncCallback) {
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
			FMarchingCubes::FParameters* PassParameters = GraphBuilder.AllocParameters<FMarchingCubes::FParameters>();
			if (Params.Input.leafCount == 0) return;

			const int maxVoxelCount = nodeVoxelCount * Params.Input.leafCount;
			const int vertexCount = maxVoxelCount * 15;
			const int triCount = maxVoxelCount * 5 * 3;

			FRDGBufferRef OutVerticesBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), vertexCount),TEXT("OutVertices_SB"));
			PassParameters->outVertices = GraphBuilder.CreateUAV(OutVerticesBuffer);

			FRDGBufferRef OutNormalsBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), vertexCount),TEXT("OutNormals_SB"));
			PassParameters->outNormals = GraphBuilder.CreateUAV(OutNormalsBuffer);

			FRDGBufferRef OutTrisBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateBufferDesc(sizeof(int32), triCount),TEXT("OutTris_SB"));
			PassParameters->outTris = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutTrisBuffer, PF_R32_SINT));

			uint32 nodeIndex = 0; 
			AddMarchingCubesGraphPassFromOctree(GraphBuilder, Params, PassParameters, Params.Input.tree, 0, &nodeIndex);

			FRHIGPUBufferReadback* VerticesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesVertices"));
			AddEnqueueCopyPass(GraphBuilder, VerticesReadback, OutVerticesBuffer, 0u);

			FRHIGPUBufferReadback* TrianglesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesIndices"));
			AddEnqueueCopyPass(GraphBuilder, TrianglesReadback, OutTrisBuffer, 0u);

			FRHIGPUBufferReadback* NormalsReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesNormals"));
			AddEnqueueCopyPass(GraphBuilder, NormalsReadback, OutNormalsBuffer, 0u);

			auto RunnerFunc = [VerticesReadback, TrianglesReadback, NormalsReadback, AsyncCallback, vertexCount, triCount](auto&& RunnerFunc) -> void {
				if (VerticesReadback->IsReady() && TrianglesReadback->IsReady() && NormalsReadback->IsReady()) {
					FMarchingCubesOutput OutVal;

					void* VBuf = VerticesReadback->Lock(0);
					OutVal.vertices.Append((FVector3f*)VBuf, vertexCount);
					VerticesReadback->Unlock();

					void* IBuf = TrianglesReadback->Lock(0);
					OutVal.tris.Append((int32*)IBuf, triCount);
					TrianglesReadback->Unlock();

					void* NBuf = NormalsReadback->Lock(0);
					OutVal.normals.Append((FVector3f*)NBuf, vertexCount);
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