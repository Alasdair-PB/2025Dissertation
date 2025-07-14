#pragma once
#include "CoreMinimal.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "RenderData.h"

#define NUM_THREADS_Deformation_X 8
#define NUM_THREADS_Deformation_Y 8
#define NUM_THREADS_Deformation_Z 8

DECLARE_STATS_GROUP(TEXT("Deformation"), STATGROUP_Deformation, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Deformation Execute"), STAT_Deformation_Execute, STATGROUP_Deformation);

class FDeformation : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDeformation);
	SHADER_USE_PARAMETER_STRUCT(FDeformation, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, subdivisionIndex)
		SHADER_PARAMETER(uint32, leafDepth)
		SHADER_PARAMETER(uint32, nodeIndex)

		SHADER_PARAMETER_SRV(Buffer<float>, isoValues)
		SHADER_PARAMETER_UAV(RWBuffer<float>, isoCombinedValues)

		SHADER_PARAMETER(uint32, voxelsPerAxis)
		SHADER_PARAMETER(uint32, highResVoxelsPerAxis)

		SHADER_PARAMETER(FVector3f, leafPosition)
		SHADER_PARAMETER(FVector3f, octreePosition)
		SHADER_PARAMETER(float, baseDepthScale)
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

void AddDeformationPass(FRDGBuilder& GraphBuilder, FVoxelComputeUpdateNodeData& nodeData, FVoxelComputeUpdateData& updateData) {

	FShaderResourceViewRHIRef baseIsoValues = updateData.isoBuffer.Get()->bufferSRV;
	FDeformation::FParameters* PassParams = GraphBuilder.AllocParameters<FDeformation::FParameters>();
	int voxelsPerAxis = updateData.voxelsPerAxis;

	PassParams->leafPosition = nodeData.boundsCenter;
	PassParams->octreePosition = updateData.octreePosition;
	PassParams->baseDepthScale = updateData.scale;
	PassParams->leafDepth = nodeData.leafDepth;
	PassParams->nodeIndex = 0;
	PassParams->subdivisionIndex = nodeData.subdivisionIndex;
	PassParams->isoValues = updateData.isoBuffer->bufferSRV;
	PassParams->isoCombinedValues = nodeData.isoBuffer->bufferUAV;
	PassParams->voxelsPerAxis = voxelsPerAxis;
	PassParams->highResVoxelsPerAxis = updateData.highResVoxelsPerAxis;

	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FDeformation> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector((voxelsPerAxis + 1)), FComputeShaderUtils::kGolden2DGroupSize);

	GraphBuilder.AddPass(RDG_EVENT_NAME("Deformation Pass"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount);  
		});
}