#pragma once
#include "CoreMinimal.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "RenderData.h"

#define NUM_THREADS_Deformation_X 8
#define NUM_THREADS_Deformation_Y 8
#define NUM_THREADS_Deformation_Z 8
/*
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
		SHADER_PARAMETER_UAV(RWBuffer<float>, outIsoValues)
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

void AddDeformationPass(FRDGBuilder& GraphBuilder, FVoxelComputeUpdateNodeData& nodeData, int voxelsPerAxis) {
	FDeformation::FParameters* PassParams = GraphBuilder.AllocParameters<FDeformation::FParameters>();
	PassParams->size = 0;
	PassParams->seed = 0;
	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FDeformation> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(PassParams->size, PassParams->size, PassParams->size), FComputeShaderUtils::kGolden2DGroupSize);

	GraphBuilder.AddPass(RDG_EVENT_NAME("Deformation Pass"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount);  
		});
}*/