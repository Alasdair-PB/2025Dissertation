#include "PlanetGeneratorDispatcher.h"
#include "MyShaders.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "MyShaders/Public/PlanetGeneratorDispatcher.h"
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

DECLARE_STATS_GROUP(TEXT("PlanetGenerator"), STATGROUP_PlanetGenerator, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("PlanetGenerator Execute"), STAT_PlanetGenerator_Execute, STATGROUP_PlanetGenerator);

class FPlanetGenerator : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FPlanetGenerator);
	SHADER_USE_PARAMETER_STRUCT(FPlanetGenerator, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, size)
		SHADER_PARAMETER(float, baseDepthScale)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, outIsoValues)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_PlanetGenerator_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_PlanetGenerator_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_PlanetGenerator_Z);
	}
};

IMPLEMENT_GLOBAL_SHADER(FPlanetGenerator, "/MyShadersShaders/PlanetGenerator.usf", "PlanetGenerator", SF_Compute);

void AddSphereGeneratorPass(FRDGBuilder& GraphBuilder, FPlanetGeneratorDispatchParams& Params, FRDGBufferUAVRef OutIsoUAV) {
	FPlanetGenerator::FParameters* PassParams = GraphBuilder.AllocParameters<FPlanetGenerator::FParameters>();
	PassParams->baseDepthScale = Params.Input.baseDepthScale;
	PassParams->size = Params.Input.size;
	PassParams->outIsoValues = OutIsoUAV;

	UE_LOG(LogTemp, Warning, TEXT(": %d"), PassParams->size);

	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FPlanetGenerator> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(PassParams->size, PassParams->size, PassParams->size), FComputeShaderUtils::kGolden2DGroupSize);

	GraphBuilder.AddPass(RDG_EVENT_NAME("Planet Generator"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount); }
	);

}

void FPlanetGeneratorInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FPlanetGeneratorDispatchParams Params, TFunction<void(FPlanetGeneratorOutput OutputVal)> AsyncCallback) {

	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_PlanetGenerator_Execute);
		DECLARE_GPU_STAT(PlanetGenerator)
		RDG_EVENT_SCOPE(GraphBuilder, "PlanetGenerator");
		RDG_GPU_STAT_SCOPE(GraphBuilder, PlanetGenerator);

		typename FPlanetGenerator::FPermutationDomain PermutationVector;
		TShaderMapRef<FPlanetGenerator> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			const int isoValueCount = Params.Input.size * Params.Input.size * Params.Input.size;

			TArray<float> OutIsoValues;
			OutIsoValues.Init(-1, isoValueCount);

			FRDGBufferRef OutIsoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"), sizeof(float), isoValueCount, OutIsoValues.GetData(), isoValueCount * sizeof(float));
			FRDGBufferUAVRef OutIsoValuesUAV = GraphBuilder.CreateUAV(OutIsoValuesBuffer);
			AddSphereGeneratorPass(GraphBuilder, Params, OutIsoValuesUAV);

			FRHIGPUBufferReadback* isoReadback = new FRHIGPUBufferReadback(TEXT("PlanetGeneratorISO"));
			AddEnqueueCopyPass(GraphBuilder, isoReadback, OutIsoValuesBuffer, 0u);

			auto RunnerFunc = [isoReadback, AsyncCallback, isoValueCount](auto&& RunnerFunc) ->
				void {
				if (isoReadback->IsReady()) {
					FPlanetGeneratorOutput OutVal;
					void* VBuf = isoReadback->Lock(0);
					OutVal.outIsoValues.Append((float*)VBuf, isoValueCount);
					isoReadback->Unlock();
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