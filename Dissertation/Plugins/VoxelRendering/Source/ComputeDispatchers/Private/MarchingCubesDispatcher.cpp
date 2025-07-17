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
#include "TextureGenerator.h"
#include "DeformationDispatcher.h"

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

		SHADER_PARAMETER_SRV(Buffer<float>, isoValues)
		SHADER_PARAMETER_SRV(Buffer<uint32>, typeValues)

		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<int>, marchLookUp)
		SHADER_PARAMETER_UAV(RWBuffer<float>, outInfo)
		SHADER_PARAMETER_UAV(RWBuffer<float>, outNormalInfo)
		SHADER_PARAMETER_UAV(RWBuffer<uint32>, outTypeInfo)

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

void AddOctreeMarchingPass(FRDGBuilder& GraphBuilder, FVoxelComputeUpdateNodeData& nodeData, FMarchingCubesDispatchParams& Params, FRDGBufferSRVRef InLookUpSRV) {
	FMarchingCubes::FParameters* PassParams = GraphBuilder.AllocParameters<FMarchingCubes::FParameters>();
	int voxelsPerAxis = Params.Input.updateData.voxelsPerAxis;

	check(nodeData.isoBuffer->bufferSRV);
	PassParams->leafPosition = nodeData.boundsCenter;
	PassParams->leafDepth = nodeData.leafDepth;
	PassParams->nodeIndex = 0; // Shader supports single vertex buffer for mesh, however as each LOD has its own vertex factory we can ignore this.
	PassParams->isoValues = nodeData.isoBuffer->bufferSRV; 
	PassParams->marchLookUp = InLookUpSRV;
	PassParams->outInfo = nodeData.vertexFactory->GetVertexUAV();
	PassParams->outNormalInfo = nodeData.vertexFactory->GetVertexNormalsUAV();
	PassParams->voxelsPerAxis = voxelsPerAxis;
	PassParams->baseDepthScale = Params.Input.updateData.scale;
	PassParams->isoLevel = Params.Input.updateData.isoLevel;

	PassParams->typeValues = nodeData.typeBuffer->bufferSRV;
	PassParams->outTypeInfo = nodeData.vertexFactory->GetVertexTypeUAV();

	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FMarchingCubes> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(voxelsPerAxis), FComputeShaderUtils::kGolden2DGroupSize);

	GraphBuilder.AddPass(RDG_EVENT_NAME("Marching Cubes"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount); });
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

		typename FDeformation::FPermutationDomain PermutationVectorDef;
		TShaderMapRef<FDeformation> DefComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVectorDef);
		bool bIsDefShaderValid = DefComputeShader.IsValid();

		if (bIsShaderValid && bIsDefShaderValid) {
			// Deformation
			for (FVoxelComputeUpdateNodeData nodeData : Params.Input.updateData.nodeData)
				AddDeformationPass(GraphBuilder, nodeData, Params.Input.updateData);

			// Marching Cubes
			FRDGBufferRef isoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"), sizeof(int), 2460, marchLookUp, 2460 * sizeof(int));
			FRDGBufferSRVRef InLookUpSRV = GraphBuilder.CreateSRV(isoValuesBuffer);

			for (FVoxelComputeUpdateNodeData nodeData : Params.Input.updateData.nodeData)
				AddOctreeMarchingPass(GraphBuilder, nodeData, Params, InLookUpSRV);

			auto RunnerFunc = [AsyncCallback](auto&& RunnerFunc) ->
				void {
					FMarchingCubesOutput OutVal;
					AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutVal]() {AsyncCallback(OutVal); });
				};
			AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
				RunnerFunc(RunnerFunc); });
		}
		else {}
	}
	GraphBuilder.Execute();
}