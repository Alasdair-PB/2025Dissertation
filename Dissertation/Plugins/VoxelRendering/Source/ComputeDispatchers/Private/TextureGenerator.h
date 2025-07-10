#pragma once
#include "CoreMinimal.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "RenderData.h"
/*
DECLARE_STATS_GROUP(TEXT("TextureGenerator"), STATGROUP_TextureGenerator, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("TextureGenerator Execute"), STAT_TextureGenerator_Execute, STATGROUP_TextureGenerator);

#define NUM_THREADS_TextureGenerator_X 8
#define NUM_THREADS_TextureGenerator_Y 8
#define NUM_THREADS_TextureGenerator_Z 8

class FTextureGenerator : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FTextureGenerator);
	SHADER_USE_PARAMETER_STRUCT(FTextureGenerator, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FVector3f, leafPosition)
		SHADER_PARAMETER(uint32, leafDepth)
		SHADER_PARAMETER(uint32, nodeIndex)
		SHADER_PARAMETER_UAV(Buffer<float>, inIsoValues)
		SHADER_PARAMETER_UAV(RWBuffer<float>, outIsoValues)
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
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_TextureGenerator_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_TextureGenerator_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_TextureGenerator_Z);
	}
};

IMPLEMENT_GLOBAL_SHADER(FTextureGenerator, "/ComputeDispatchersShaders/TextureGenerator.usf", "TextureGenerator", SF_Compute);

void TextureGeneratorPass(FRDGBuilder& GraphBuilder, FVoxelComputeUpdateNodeData& nodeData, int voxelsPerAxis) {

	FTextureGenerator::FParameters* PassParams = GraphBuilder.AllocParameters<FTextureGenerator::FParameters>();

	PassParams->leafPosition = nodeData.boundsCenter;
	PassParams->leafDepth = nodeData.leafDepth;
	PassParams->nodeIndex = 0;

	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FTextureGenerator> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(voxelsPerAxis), FComputeShaderUtils::kGolden2DGroupSize);

	GraphBuilder.AddPass(RDG_EVENT_NAME("Marching Cubes"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount); });
}*/
