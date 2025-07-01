#include "MarchingCubesDispatcher.h"
#include "ComputeDispatchers.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "ComputeDispatchers/Public/MarchingCubesDispatcher.h"
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
#include "VoxelBufferUtils.h"

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
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float>, isoValues)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<int>, marchLookUp)
		SHADER_PARAMETER_UAV(RWBuffer<float3>, outInfo)
		SHADER_PARAMETER_UAV(RWBuffer<float3>, outNormalInfo)
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
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_MarchingCubes_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_MarchingCubes_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_MarchingCubes_Z);
	}
};

IMPLEMENT_GLOBAL_SHADER(FMarchingCubes, "/ComputeDispatchersShaders/MarchingCubes.usf", "MarchingCubes", SF_Compute);

void AddOctreeMarchingPass(FRDGBuilder& GraphBuilder, OctreeNode* node, uint32 depth, uint32* nodeIndex, FMarchingCubesDispatchParams& Params, FRDGBufferSRVRef InLookUpSRV) {

	if (!node) return;
	if (!(node->isLeaf)) {	
		depth++;
		for (OctreeNode* child : node->children)
			AddOctreeMarchingPass(GraphBuilder, child, depth, nodeIndex, Params, InLookUpSRV);
		return;
	}

	const FRDGBufferRef isoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"), sizeof(float), isoCount, node->isoValues, isoCount * sizeof(float));
	FMarchingCubes::FParameters* PassParams = GraphBuilder.AllocParameters<FMarchingCubes::FParameters>();
	PassParams->leafPosition = node->bounds.Center();
	PassParams->leafDepth = depth;
	PassParams->nodeIndex = (*nodeIndex);
	PassParams->isoValues = GraphBuilder.CreateSRV(isoValuesBuffer);
	PassParams->marchLookUp = InLookUpSRV;
	PassParams->outInfo = Params.Input.vertexBufferRHIRef.VertexInfoRHIRef;
	PassParams->outNormalInfo = Params.Input.vertexBufferRHIRef.vertexNormalsInfoRHIRef;
	PassParams->voxelsPerAxis = voxelsPerAxis;
	PassParams->baseDepthScale = Params.Input.baseDepthScale;
	PassParams->isoLevel = Params.Input.isoLevel;

	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FMarchingCubes> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(voxelsPerAxis), FComputeShaderUtils::kGolden2DGroupSize);

	GraphBuilder.AddPass(RDG_EVENT_NAME("Marching Cubes"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount); });
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
			FRDGBufferRef isoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"), sizeof(int), 2460, marchLookUp, 2460 * sizeof(int));
			FRDGBufferSRVRef InLookUpSRV = GraphBuilder.CreateSRV(isoValuesBuffer);
			//UE_LOG(LogTemp, Warning, TEXT("Shader dispatched"));

			uint32 vertexCount = Params.Input.vertexBufferRHIRef.NumElements;

			//FRDGBufferDesc vertexBufferDec = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVoxelVertexInfo), vertexNumElements);
			//TRefCountPtr<FRDGPooledBuffer> vertexPooledBuffer = new FRDGPooledBuffer(RHICmdList, Params.Input.vertexBufferRHIRef.BufferRHI, vertexBufferDec, vertexNumElements, TEXT("VoxelVertexPooledBuffer"));
			//FRDGBufferRef RDGVertexBuffer = GraphBuilder.RegisterExternalBuffer(vertexPooledBuffer, TEXT("VoxelIndexBuffer"));

			//FRDGBuffer* VertexToEdgeBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateStructuredDesc(sizeof(FVoxelVertexInfo), vertexNumElements), TEXT("Voxel.VertexToEdge"));
			//GraphBuilder.QueueBufferUpload(VertexToEdgeBuffer, Params.Input.vertexBufferRHIRef.BufferRHI, vertexNumElements * sizeof(FVoxelVertexInfo));
			//TRefCountPtr<FRDGPooledBuffer> vertexPooledBuffer = GraphBuilder.ConvertToExternalBuffer(VertexToEdgeBuffer);
			//FRDGBufferRef RDGVertexBuffer = GraphBuilder.RegisterExternalBuffer(vertexPooledBuffer, TEXT("VoxelVertexBuffer"));

			uint32 nodeIndex = 0;
			AddOctreeMarchingPass(GraphBuilder, Params.Input.tree, 0, &nodeIndex, Params, InLookUpSRV);

			auto RunnerFunc = [AsyncCallback](auto&& RunnerFunc) ->
				void {
					FMarchingCubesOutput OutVal;
					AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutVal]() {AsyncCallback(OutVal); });
				};
			AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
				RunnerFunc(RunnerFunc); });
		}
		else {}

			/*FRHIGPUBufferReadback* VerticesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesVertices"));
			FRHIGPUBufferReadback* TrianglesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesIndices"));
			FRHIGPUBufferReadback* NormalsReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesNormals"));

			AddEnqueueCopyPass(GraphBuilder, NormalsReadback, OutNormalsBuffer, 0u);
			AddEnqueueCopyPass(GraphBuilder, VerticesReadback, RDGVertexBuffer, 0u);
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
		else {}*/
	}
	GraphBuilder.Execute();
}