#include "MarchingCubesDispatcher.h"
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
	RDG_BUFFER_ACCESS(source, ERHIAccess::CopySrc)
END_SHADER_PARAMETER_STRUCT()

class FMarchingCubes : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMarchingCubes);
	SHADER_USE_PARAMETER_STRUCT(FMarchingCubes, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float>, isoValues)
		SHADER_PARAMETER(FVector3f, leafPosition)
		SHADER_PARAMETER(uint32, leafDepth)

		SHADER_PARAMETER(uint32, voxelsPerNode)
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
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_MySimpleComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_MySimpleComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_MySimpleComputeShader_Z);
	}
};

static void AddExtractBufferPass(FRDGBuilder& GraphBuilder, const FRDGBufferRef source, void* path, SIZE_T size)
{
	FExtractBufferParameters* Params = GraphBuilder.AllocParameters<FExtractBufferParameters>();
	Params->source = source;
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("PooledBufferDownload(%s)", source->Name), Params, ERDGPassFlags::Copy | ERDGPassFlags::NeverCull,
		[Params, path, size](FRHICommandListImmediate& RHICmdList)
		{
			void* PSource = RHICmdList.LockBuffer(Params->source->GetRHI(), 0, size, RLM_ReadOnly);
			FMemory::Memcpy(path, PSource, size);
			RHICmdList.UnlockBuffer(Params->source->GetRHI());
		}
	);
}

IMPLEMENT_GLOBAL_SHADER(FMarchingCubes, "/MyShadersShaders/MarchingCubes.usf", "MarchingCubes", SF_Compute);
void AddMarchingCubesGraphPass(FRDGBuilder& GraphBuilder, const FMarchingCubesDispatchParams Params) {

	FMarchingCubes::FParameters* PassParameters = GraphBuilder.AllocParameters<FMarchingCubes::FParameters>();
	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FMarchingCubes> ComputeShader(ShaderMap);

	FMarchingCubes::FParameters* CSParameters = GraphBuilder.AllocParameters<FMarchingCubes::FParameters>();
	CSParameters->isoValues = PassParameters->isoValues;
	CSParameters->leafDepth = PassParameters->leafDepth;
	CSParameters->leafPosition = PassParameters->leafPosition;

	CSParameters->voxelsPerNode = PassParameters->voxelsPerNode;
	CSParameters->nodeIndex = PassParameters->nodeIndex;
	CSParameters->baseDepthScale = PassParameters->baseDepthScale;
	CSParameters->isoLevel = PassParameters->isoLevel;

	CSParameters->outVertices = PassParameters->outVertices;
	CSParameters->outTris = PassParameters->outTris;
	CSParameters->outNormals = PassParameters->outNormals;

	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), FComputeShaderUtils::kGolden2DGroupSize);
	GraphBuilder.AddPass(RDG_EVENT_NAME("Marching Cubes"), CSParameters, ERDGPassFlags::AsyncCompute,
		[&CSParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *CSParameters, GroupCount); }
	);
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

			const int maxVoxelCount = Params.Input.voxelsPerNode;
			const int vertexCount = maxVoxelCount * 15;
			const int triCount = maxVoxelCount * 5 * 3;
			const int isoCount = maxVoxelCount * 8;

			TArray<FVector3f> outVertices;
			TArray<int32> outTris;
			TArray<FVector3f> outNormals;

			outVertices.Init(FVector3f(-1, -1, -1), vertexCount);
			outTris.Init(-1, triCount);
			outNormals.Init(FVector3f(0, 0, 0), vertexCount);

			const FRDGBufferRef outVerticesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("OutVertices_SB"),
				sizeof(FVector3f), vertexCount, outVertices.GetData(), vertexCount * sizeof(FVector3f));
			PassParameters->outVertices = GraphBuilder.CreateUAV(outVerticesBuffer);

			const FRDGBufferRef outTrisBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("OutTris_SB"),
				sizeof(int32), triCount, outTris.GetData(), triCount * sizeof(int32));
			PassParameters->outTris = GraphBuilder.CreateUAV(outTrisBuffer);

			const FRDGBufferRef outNormalsBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("OutNormals_SB"),
				sizeof(FVector3f), vertexCount, outNormals.GetData(), vertexCount * sizeof(FVector3f));
			PassParameters->outNormals = GraphBuilder.CreateUAV(outNormalsBuffer);

			const FRDGBufferRef isoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("GridNormals_SB"),
				sizeof(FVector3f), maxVoxelCount, nullptr, 0);
			PassParameters->isoValues = GraphBuilder.CreateSRV(isoValuesBuffer);

			/*for (int i = 0; i < Params.Input.leafNodes.Num(); ++i) {
				FMarchingCubesDispatchParams NodeParams = Params;
				NodeParams.Input.nodeIndex = i;
				AddMarchingCubesGraphPass(GraphBuilder, NodeParams);
			}*/
			AddMarchingCubesGraphPass(GraphBuilder, Params);

			FRHIGPUBufferReadback* VerticesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesVertices"));
			AddEnqueueCopyPass(GraphBuilder, VerticesReadback, outVerticesBuffer, 0u);

			FRHIGPUBufferReadback* TrianglesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesIndices"));
			AddEnqueueCopyPass(GraphBuilder, TrianglesReadback, outTrisBuffer, 0u);

			FRHIGPUBufferReadback* NormalsReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesNormals"));
			AddEnqueueCopyPass(GraphBuilder, NormalsReadback, outNormalsBuffer, 0u);

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

					AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutVal]() {
						AsyncCallback(OutVal); });

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

			AsyncTask(ENamedThreads::GameThread, [AsyncCallback, outVertices, outTris, outNormals]() {
				FMarchingCubesOutput OutVal;
				OutVal.vertices = outVertices;
				OutVal.tris = outTris;
				OutVal.normals = outNormals;
				AsyncCallback(OutVal);
				});

		} else {} // We silently exit here as we don't want to crash the game if the shader is not found or has an error.
	}

	GraphBuilder.Execute();
}