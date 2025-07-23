#pragma once
#include "CoreMinimal.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "RenderData.h"

#define NUM_THREADS_TransvoxelDeformation_X 8
#define NUM_THREADS_TransvoxelDeformation_Y 8
#define NUM_THREADS_TransvoxelDeformation_Z 8

DECLARE_STATS_GROUP(TEXT("TransvoxelDeformation"), STATGROUP_TVDeformation, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("TransvoxelDeformation Execute"), STATGROUP_TVDeformation, STATGROUP_TVDeformation);

class FTransvoxelDeformation : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FTransvoxelDeformation);
	SHADER_USE_PARAMETER_STRUCT(FTransvoxelDeformation, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, leafDepth)
		SHADER_PARAMETER(uint32, nodeIndex)

		SHADER_PARAMETER_SRV(Buffer<float>, isoValues)
		SHADER_PARAMETER_SRV(Buffer<float>, isoDeltaValues)
		SHADER_PARAMETER_UAV(RWBuffer<float>, isoCombinedValues)

		SHADER_PARAMETER_SRV(Buffer<uint32>, typeValues)
		SHADER_PARAMETER_SRV(Buffer<uint32>, typeDeltaValues)
		SHADER_PARAMETER_UAV(RWBuffer<uint32>, typeCombinedValues)

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
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_TransvoxelDeformation_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_TransvoxelDeformation_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_TransvoxelDeformation_Z);
	}
};

IMPLEMENT_GLOBAL_SHADER(FTransvoxelDeformation, "/ComputeDispatchersShaders/TransvoxelDeformation.usf", "TransvoxelDeformation", SF_Compute);

void AddTransvoxelDeformationPass(FRDGBuilder& GraphBuilder, FVoxelComputeUpdateNodeData& nodeData, FVoxelComputeUpdateData& updateData) {

	FShaderResourceViewRHIRef baseIsoValues = updateData.isoBuffer.Get()->bufferSRV;
	FTransvoxelDeformation::FParameters* PassParams = GraphBuilder.AllocParameters<FTransvoxelDeformation::FParameters>();
	int voxelsPerAxis = updateData.voxelsPerAxis;

	PassParams->leafPosition = nodeData.boundsCenter;
	PassParams->octreePosition = updateData.octreePosition;
	PassParams->baseDepthScale = updateData.scale;
	PassParams->leafDepth = nodeData.leafDepth;
	PassParams->nodeIndex = 0;

	PassParams->isoValues = updateData.isoBuffer->bufferSRV;
	PassParams->isoDeltaValues = updateData.deltaIsoBuffer->bufferSRV;
	PassParams->isoCombinedValues = nodeData.isoBuffer->bufferUAV;

	PassParams->typeValues = updateData.typeBuffer->bufferSRV;
	PassParams->typeDeltaValues = updateData.deltaTypeBuffer->bufferSRV;
	PassParams->typeCombinedValues = nodeData.typeBuffer->bufferUAV;

	PassParams->voxelsPerAxis = voxelsPerAxis;
	PassParams->highResVoxelsPerAxis = updateData.highResVoxelsPerAxis;

	int isoValuesPerAxis = voxelsPerAxis + 1;
	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FTransvoxelDeformation> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector((isoValuesPerAxis)), FComputeShaderUtils::kGolden2DGroupSize);

	GraphBuilder.AddPass(RDG_EVENT_NAME("Transvoxel Deformation Pass"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount);  
		});
}