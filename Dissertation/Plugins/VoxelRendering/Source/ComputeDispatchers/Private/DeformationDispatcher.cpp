#include "DeformationDispatcher.h"
#include "ComputeDispatchers.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "ComputeDispatchers/Public/DeformationDispatcher.h"
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

DECLARE_STATS_GROUP(TEXT("Deformation"), STATGROUP_Deformation, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Deformation Execute"), STAT_Deformation_Execute, STATGROUP_Deformation);

class FDeformation : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDeformation);
	SHADER_USE_PARAMETER_STRUCT(FDeformation, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, size)
		SHADER_PARAMETER(uint32, seed)
		SHADER_PARAMETER(float, baseDepthScale)
		SHADER_PARAMETER(float, planetScaleRatio)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, outIsoValues)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_Deformation_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_Deformation_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_Deformation_Z);
	}
};

IMPLEMENT_GLOBAL_SHADER(FDeformation, "/ComputeDispatchersShaders/Deformation.usf", "Deformation", SF_Compute);


void AddDeformationPass(FRDGBuilder& GraphBuilder, FDeformationDispatchParams& Params, FRDGBufferUAVRef OutTypeUAV, FRDGBufferSRVRef InIsoSRV) {
	FDeformation::FParameters* PassParams = GraphBuilder.AllocParameters<FDeformation::FParameters>();
	PassParams->size = Params.Input.size;
	PassParams->seed = Params.Input.seed;


	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FDeformation> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(PassParams->size, PassParams->size, PassParams->size), FComputeShaderUtils::kGolden2DGroupSize);

	GraphBuilder.AddPass(RDG_EVENT_NAME("Planet Biome Generator"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount); }
	);
}

void FDeformationInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FDeformationDispatchParams Params, TFunction<void(FDeformationOutput OutputVal)> AsyncCallback) {

	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_Deformation_Execute);
		DECLARE_GPU_STAT(Deformation)
		RDG_EVENT_SCOPE(GraphBuilder, "Deformation");
		RDG_GPU_STAT_SCOPE(GraphBuilder, Deformation);

		typename FDeformation::FPermutationDomain DeformationPermutationVector;
		TShaderMapRef<FDeformation> DeformationComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), DeformationPermutationVector);
		bool bIsShaderValid = DeformationComputeShader.IsValid();

		if (bIsShaderValid) {
			const int isoValueCount = Params.Input.size * Params.Input.size * Params.Input.size;

			TArray<float> OutIsoValues;
			TArray<float> OutTypeValues;

			OutIsoValues.Init(-1, isoValueCount);
			OutTypeValues.Init(-1, isoValueCount);

			FRDGBufferRef OutIsoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"), sizeof(float), isoValueCount, OutIsoValues.GetData(), isoValueCount * sizeof(float));
			FRDGBufferRef OutTypeValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"), sizeof(int), isoValueCount, OutIsoValues.GetData(), isoValueCount * sizeof(int));

			FRDGBufferUAVRef OutIsoValuesUAV = GraphBuilder.CreateUAV(OutIsoValuesBuffer);
			FRDGBufferUAVRef OutTypeValuesUAV = GraphBuilder.CreateUAV(OutTypeValuesBuffer);
			FRDGBufferSRVRef IsoValuesSRV = GraphBuilder.CreateSRV(OutIsoValuesBuffer);

			AddDeformationPass(GraphBuilder, Params, OutTypeValuesUAV, IsoValuesSRV);

			FRHIGPUBufferReadback* isoReadback = new FRHIGPUBufferReadback(TEXT("DeformationISO"));
			FRHIGPUBufferReadback* typeReadback = new FRHIGPUBufferReadback(TEXT("DeformationTYPE"));

			AddEnqueueCopyPass(GraphBuilder, isoReadback, OutIsoValuesBuffer, 0u);
			AddEnqueueCopyPass(GraphBuilder, typeReadback, OutTypeValuesBuffer, 0u);

			auto RunnerFunc = [isoReadback, typeReadback, AsyncCallback, isoValueCount](auto&& RunnerFunc) ->
				void {
				if (isoReadback->IsReady() && typeReadback->IsReady()) {
					FDeformationOutput OutVal;

					void* VBuf = isoReadback->Lock(0);
					OutVal.outIsoValues.Append((float*)VBuf, isoValueCount);
					isoReadback->Unlock();

					void* VTypeBuf = typeReadback->Lock(0);
					OutVal.outTypeValues.Append((uint32*)VTypeBuf, isoValueCount);
					typeReadback->Unlock();

					AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutVal]() {AsyncCallback(OutVal); });
					delete isoReadback;
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