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
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FVector>, voxelBodyCoord)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<int>, scalarFieldOffset)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<int>, depthLevel)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float>, scalarField)

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
		OutEnvironment.SetDefine(TEXT("voxelResolutionPerAxis"), NUM_THREADS_MySimpleComputeShader_X);
		OutEnvironment.SetDefine(TEXT("voxelResolutionPerAxis"), NUM_THREADS_MySimpleComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("voxelResolutionPerAxis"), NUM_THREADS_MySimpleComputeShader_Z);
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
	CSParameters->voxelBodyCoord = PassParameters->voxelBodyCoord;
	CSParameters->scalarFieldOffset = PassParameters->scalarFieldOffset;
	CSParameters->depthLevel = PassParameters->depthLevel;
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

			const int vertexCount = 0;
			const int cellCount = 0;
			const int triCount = 0;

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

			const FRDGBufferRef voxelBodyCoordBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("GridNormals_SB"),
				sizeof(FVector3f), Params.voxelBodyDimensions.X * Params.voxelBodyDimensions.Y * Params.voxelBodyDimensions.Z, nullptr, 0);
			PassParameters->voxelBodyCoord = GraphBuilder.CreateSRV(voxelBodyCoordBuffer);

			const FRDGBufferRef depthLevelBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("GridNormals_SB"),
				sizeof(FVector3f), Params.voxelBodyDimensions.X * Params.voxelBodyDimensions.Y * Params.voxelBodyDimensions.Z, nullptr, 0);
			PassParameters->depthLevel = GraphBuilder.CreateSRV(depthLevelBuffer);

			const FRDGBufferRef scalarFieldOffsetBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("GridNormals_SB"),
				sizeof(FVector3f), Params.voxelBodyDimensions.X * Params.voxelBodyDimensions.Y * Params.voxelBodyDimensions.Z, nullptr, 0);
			PassParameters->scalarFieldOffset = GraphBuilder.CreateSRV(scalarFieldOffsetBuffer);

			AddMarchingCubesGraphPass(GraphBuilder, Params);

			FRHIGPUBufferReadback* VerticesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesVertices"));
			AddEnqueueCopyPass(GraphBuilder, VerticesReadback, outVerticesBuffer, 0u);

			FRHIGPUBufferReadback* TrianglesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesIndices"));
			AddEnqueueCopyPass(GraphBuilder, TrianglesReadback, outTrisBuffer, 0u);

			FRHIGPUBufferReadback* NormalsReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesNormals"));
			AddEnqueueCopyPass(GraphBuilder, NormalsReadback, outNormalsBuffer, 0u);

			auto RunnerFunc = [VerticesReadback, TrianglesReadback, NormalsReadback, AsyncCallback](auto&& RunnerFunc) -> void {
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