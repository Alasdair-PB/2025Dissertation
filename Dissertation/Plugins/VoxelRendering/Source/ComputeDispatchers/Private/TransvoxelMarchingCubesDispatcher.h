#pragma once
#include "CoreMinimal.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "RenderData.h"

#define NUM_THREADS_TransvoxelMC_X 8
#define NUM_THREADS_TransvoxelMC_Y 8
#define NUM_THREADS_TransvoxelMC_Z 8

DECLARE_STATS_GROUP(TEXT("TransvoxelMC"), STATGROUP_TransVoxel, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("TransvoxelMC Execute"), STAT_TransVoxel_Execute, STATGROUP_TransVoxel);

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

		SHADER_PARAMETER_UAV(RWBuffer<float>, outVertexInfo)
		SHADER_PARAMETER_UAV(RWBuffer<float>, outNormalInfo)
		SHADER_PARAMETER_UAV(RWBuffer<uint32>, outTypeInfo)

		SHADER_PARAMETER(uint32, voxelsPerAxis)
		SHADER_PARAMETER(float, baseDepthScale)
		SHADER_PARAMETER(float, isoLevel)
		SHADER_PARAMETER(FIntVector, direction)
		SHADER_PARAMETER(uint32, transitionCellIndex)

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

IMPLEMENT_GLOBAL_SHADER(FTransvoxelMC, "/ComputeDispatchersShaders/TransvoxelMarchingCubes.usf", "TransvoxelMarchingCubes", SF_Compute);

void AddTransvoxelMarchingCubesPass(FRDGBuilder& GraphBuilder, FVoxelTransVoxelNodeData& transVoxelNodeData, FVoxelComputeUpdateData& updateData) {
	
	FTransvoxelMC::FParameters* PassParams = GraphBuilder.AllocParameters<FTransvoxelMC::FParameters>();
	FVoxelComputeUpdateNodeData& nodeData = transVoxelNodeData.lowResolutionData;
	int voxelsPerAxis = updateData.voxelsPerAxis;

	check(nodeData.isoBuffer->bufferSRV);
	PassParams->leafPosition = nodeData.boundsCenter;
	PassParams->leafDepth = nodeData.leafDepth;
	PassParams->nodeIndex = 0; // Shader supports single vertex buffer for mesh, however as each LOD has its own vertex factory we can ignore this.

	PassParams->isoValues = nodeData.isoBuffer->bufferSRV;
	PassParams->typeValues = nodeData.typeBuffer->bufferSRV;

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

	check(nodeData.vertexFactory->GetVertexUAV());
	PassParams->outVertexInfo = nodeData.vertexFactory->GetVertexUAV();
	PassParams->outNormalInfo = nodeData.vertexFactory->GetVertexNormalsUAV();
	PassParams->outTypeInfo = nodeData.vertexFactory->GetVertexTypeUAV();

	PassParams->voxelsPerAxis = voxelsPerAxis;

	PassParams->baseDepthScale = updateData.scale;
	PassParams->isoLevel = updateData.isoLevel;
	PassParams->direction = transVoxelNodeData.direction;
	PassParams->transitionCellIndex = transVoxelNodeData.transitionCellIndex;

	int isoValuesPerAxis = voxelsPerAxis + 1;
	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FTransvoxelMC> ComputeShader(ShaderMap);
	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector((isoValuesPerAxis, isoValuesPerAxis, 1)), FComputeShaderUtils::kGolden2DGroupSize);

	GraphBuilder.AddPass(RDG_EVENT_NAME("TransvoxelMC Pass"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount);  
		});
}