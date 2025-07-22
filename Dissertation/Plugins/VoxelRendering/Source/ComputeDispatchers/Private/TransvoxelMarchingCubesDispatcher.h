#pragma once
#include "CoreMinimal.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "RenderData.h"

#define NUM_THREADS_TransvoxelMC_X 8
#define NUM_THREADS_TransvoxelMC_Y 8
#define NUM_THREADS_TransvoxelMC_Z 8

DECLARE_STATS_GROUP(TEXT("TransvoxelMC"), STATGROUP_Deformation, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("TransvoxelMC Execute"), STAT_Deformation_Execute, STATGROUP_Deformation);

class FTransvoxelMC : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FTransvoxelMC);
	SHADER_USE_PARAMETER_STRUCT(FTransvoxelMC, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FVector3f, leafPosition)
		SHADER_PARAMETER(uint32, leafDepth)
		SHADER_PARAMETER(uint32, nodeIndex)

		SHADER_PARAMETER_SRV(Buffer<float>, isoValues)
		SHADER_PARAMETER_SRV(Buffer<uint32>, typeValues)

		SHADER_PARAMETER_SRV(Buffer<float>, isoAdjValuesA)
		SHADER_PARAMETER_SRV(Buffer<float>, isoAdjValuesB)
		SHADER_PARAMETER_SRV(Buffer<float>, isoAdjValuesC)
		SHADER_PARAMETER_SRV(Buffer<float>, isoAdjValuesD)

		SHADER_PARAMETER_SRV(Buffer<uint32>, typeAdjValuesA)
		SHADER_PARAMETER_SRV(Buffer<uint32>, typeAdjValuesB)
		SHADER_PARAMETER_SRV(Buffer<uint32>, typeAdjValuesC)
		SHADER_PARAMETER_SRV(Buffer<uint32>, typeAdjValuesD)

		SHADER_PARAMETER_SRV(Buffer<uint32>, transitionLookup)
		SHADER_PARAMETER_SRV(Buffer<uint32>, flatTransitionVertexData)

		SHADER_PARAMETER_UAV(RWBuffer<float>, outInfo)
		SHADER_PARAMETER_UAV(RWBuffer<float>, outNormalInfo)
		SHADER_PARAMETER_UAV(RWBuffer<uint32>, outTypeInfo)

		SHADER_PARAMETER(uint32, voxelsPerAxis)
		SHADER_PARAMETER(float, baseDepthScale)
		SHADER_PARAMETER(float, isoLevel)
		SHADER_PARAMETER(uint32, direction)

	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_TransvoxelMC_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_TransvoxelMC_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_TransvoxelMC_Z);
	}
};

IMPLEMENT_GLOBAL_SHADER(FDeformation, "/ComputeDispatchersShaders/TransvoxelMarchingCubesDispatcher.usf", "TransvoxelMarchingCubesDispatcher", SF_Compute);

void AddDeformationPass(FRDGBuilder& GraphBuilder, FVoxelComputeUpdateNodeData& nodeData, FVoxelTransVoxelNodeData& transVoxelNodeData, FVoxelComputeUpdateData& updateData) {

	FShaderResourceViewRHIRef baseIsoValues = updateData.isoBuffer.Get()->bufferSRV;
	FTransvoxelMC::FParameters* PassParams = GraphBuilder.AllocParameters<FTransvoxelMC::FParameters>();
	int voxelsPerAxis = updateData.voxelsPerAxis;

	check(nodeData.isoBuffer->bufferSRV);
	PassParams->leafPosition = nodeData.boundsCenter;
	PassParams->leafDepth = nodeData.leafDepth;
	PassParams->nodeIndex = 0; // Shader supports single vertex buffer for mesh, however as each LOD has its own vertex factory we can ignore this.

	PassParams->isoValues = transVoxelNodeData.lowResolutionData.isoBuffer->bufferSRV;
	PassParams->typeValues = transVoxelNodeData.lowResolutionData.typeBuffer->bufferSRV;

	PassParams->isoAdjValuesA = transVoxelNodeData.highResolutionData[0].isoBuffer->bufferSRV;
	PassParams->isoAdjValuesB = transVoxelNodeData.highResolutionData[1].isoBuffer->bufferSRV;
	PassParams->isoAdjValuesC = transVoxelNodeData.highResolutionData[2].isoBuffer->bufferSRV;
	PassParams->isoAdjValuesD = transVoxelNodeData.highResolutionData[3].isoBuffer->bufferSRV;

	PassParams->typeAdjValuesA = transVoxelNodeData.highResolutionData[0].typeBuffer->bufferSRV;
	PassParams->typeAdjValuesB = transVoxelNodeData.highResolutionData[1].typeBuffer->bufferSRV;
	PassParams->typeAdjValuesC = transVoxelNodeData.highResolutionData[2].typeBuffer->bufferSRV;
	PassParams->typeAdjValuesD = transVoxelNodeData.highResolutionData[3].typeBuffer->bufferSRV;

	PassParams->transitionLookup = updateData.marchLookUpResource->transVoxelLookUpBufferSRV;
	PassParams->flatTransitionVertexData = updateData.marchLookUpResource->transVoxelVertexLookUpBufferSRV;

	PassParams->outInfo = nodeData.vertexFactory->GetVertexUAV();
	PassParams->outNormalInfo = nodeData.vertexFactory->GetVertexNormalsUAV();
	PassParams->voxelsPerAxis = voxelsPerAxis;
	PassParams->baseDepthScale = updateData.scale;
	PassParams->isoLevel = updateData.isoLevel;

	PassParams->direction = transVoxelNodeData.direction;

	PassParams->outTypeInfo = nodeData.vertexFactory->GetVertexTypeUAV();
	int isoValuesPerAxis = voxelsPerAxis + 1;
	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FTransvoxelMC> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector((isoValuesPerAxis)), FComputeShaderUtils::kGolden2DGroupSize);

	GraphBuilder.AddPass(RDG_EVENT_NAME("TransvoxelMC Pass"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount);  
		});
}