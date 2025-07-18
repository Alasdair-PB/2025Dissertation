#include "PlanetGeneratorDispatcher.h"
#include "ComputeDispatchers.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "ComputeDispatchers/Public/PlanetGeneratorDispatcher.h"
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

class FPlanetNoiseGenerator : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FPlanetNoiseGenerator);
	SHADER_USE_PARAMETER_STRUCT(FPlanetNoiseGenerator, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, size)
		SHADER_PARAMETER(uint32, seed)
		SHADER_PARAMETER(float, baseDepthScale)
		SHADER_PARAMETER(float, planetScaleRatio)
		SHADER_PARAMETER(float, isoLevel)
		SHADER_PARAMETER(float, fbmAmplitude)
		SHADER_PARAMETER(float, fbmFrequency)
		SHADER_PARAMETER(float, voronoiScale)
		SHADER_PARAMETER(float, voronoiJitter)
		SHADER_PARAMETER(float, voronoiWeight)
		SHADER_PARAMETER(float, voronoiThreshold)

		SHADER_PARAMETER(float, fbmWeight)
		SHADER_PARAMETER(float, surfaceWeight)
		SHADER_PARAMETER(int, surfaceLayers)
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

class FPlanetBiomeGenerator : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FPlanetBiomeGenerator);
	SHADER_USE_PARAMETER_STRUCT(FPlanetBiomeGenerator, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, size)
		SHADER_PARAMETER(uint32, seed)
		SHADER_PARAMETER(float, isoLevel)
		SHADER_PARAMETER(float, baseDepthScale)
		SHADER_PARAMETER(float, planetScaleRatio)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float>, isoValues)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, outTypeValues)
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

IMPLEMENT_GLOBAL_SHADER(FPlanetNoiseGenerator, "/ComputeDispatchersShaders/PlanetNoiseGenerator.usf", "PlanetNoiseGenerator", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FPlanetBiomeGenerator, "/ComputeDispatchersShaders/PlanetBiomeGenerator.usf", "PlanetBiomeGenerator", SF_Compute);

void AddSphereGeneratorPass(FRDGBuilder& GraphBuilder, FPlanetGeneratorDispatchParams& Params, FRDGBufferUAVRef OutIsoUAV) {
	FPlanetNoiseGenerator::FParameters* PassParams = GraphBuilder.AllocParameters<FPlanetNoiseGenerator::FParameters>();
	PassParams->size = Params.Input.size;
	PassParams->seed = Params.Input.seed;
	PassParams->baseDepthScale = Params.Input.baseDepthScale;
	PassParams->planetScaleRatio = Params.Input.planetScaleRatio;
	PassParams->outIsoValues = OutIsoUAV;
	PassParams->isoLevel = Params.Input.isoLevel;
	PassParams->fbmAmplitude = Params.Input.fbmAmplitude;
	PassParams->fbmFrequency = Params.Input.fbmFrequency;
	PassParams->voronoiScale = Params.Input.voronoiScale;
	PassParams->voronoiJitter = Params.Input.voronoiJitter;
	PassParams->voronoiWeight = Params.Input.voronoiWeight;
	PassParams->fbmWeight = Params.Input.fbmWeight;
	PassParams->surfaceWeight = Params.Input.surfaceWeight;
	PassParams->surfaceLayers = Params.Input.surfaceLayers;
	PassParams->voronoiThreshold = Params.Input.voronoiThreshold;

	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FPlanetNoiseGenerator> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(PassParams->size, PassParams->size, PassParams->size), FComputeShaderUtils::kGolden2DGroupSize);

	GraphBuilder.AddPass(RDG_EVENT_NAME("Planet Noise Generator"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount); }
	);

}

void AddBiomeGeneratorPass(FRDGBuilder& GraphBuilder, FPlanetGeneratorDispatchParams& Params, FRDGBufferUAVRef OutTypeUAV, FRDGBufferSRVRef InIsoSRV) {
	FPlanetBiomeGenerator::FParameters* PassParams = GraphBuilder.AllocParameters<FPlanetBiomeGenerator::FParameters>();
	PassParams->size = Params.Input.size;
	PassParams->seed = Params.Input.seed;
	PassParams->isoLevel = Params.Input.isoLevel;
	PassParams->baseDepthScale = Params.Input.baseDepthScale;
	PassParams->planetScaleRatio = Params.Input.planetScaleRatio;
	PassParams->isoValues = InIsoSRV;
	PassParams->outTypeValues = OutTypeUAV;
	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FPlanetBiomeGenerator> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(PassParams->size, PassParams->size, PassParams->size), FComputeShaderUtils::kGolden2DGroupSize);

	GraphBuilder.AddPass(RDG_EVENT_NAME("Planet Biome Generator"), PassParams, ERDGPassFlags::AsyncCompute,
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

		typename FPlanetNoiseGenerator::FPermutationDomain NoisePermutationVector;
		TShaderMapRef<FPlanetNoiseGenerator> NoiseComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), NoisePermutationVector);
		bool bIsNoiseShaderValid = NoiseComputeShader.IsValid();

		typename FPlanetBiomeGenerator::FPermutationDomain BiomePermutationVector;
		TShaderMapRef<FPlanetBiomeGenerator> BiomeComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), BiomePermutationVector);
		bool bIsBiomeShaderValid = BiomeComputeShader.IsValid();

		if (bIsNoiseShaderValid && bIsBiomeShaderValid) {
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

			AddSphereGeneratorPass(GraphBuilder, Params, OutIsoValuesUAV);
			AddBiomeGeneratorPass(GraphBuilder, Params, OutTypeValuesUAV, IsoValuesSRV);

			FRHIGPUBufferReadback* isoReadback = new FRHIGPUBufferReadback(TEXT("PlanetGeneratorISO"));
			FRHIGPUBufferReadback* typeReadback = new FRHIGPUBufferReadback(TEXT("PlanetGeneratorTYPE"));

			AddEnqueueCopyPass(GraphBuilder, isoReadback, OutIsoValuesBuffer, 0u);
			AddEnqueueCopyPass(GraphBuilder, typeReadback, OutTypeValuesBuffer, 0u);

			auto RunnerFunc = [isoReadback, typeReadback, AsyncCallback, isoValueCount](auto&& RunnerFunc) ->
				void {
				if (isoReadback->IsReady() && typeReadback->IsReady()) {
					FPlanetGeneratorOutput OutVal;

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