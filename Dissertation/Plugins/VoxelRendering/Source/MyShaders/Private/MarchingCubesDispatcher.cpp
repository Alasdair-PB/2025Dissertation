#include "MarchingCubesDispatcher.h"
#include "MyShaders.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "MyShaders/Public/MarchingCubesDispatcher.h"
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

DECLARE_STATS_GROUP(TEXT("MarchingCubes"), STATGROUP_MarchingCubes, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("MarchingCubes Execute"), STAT_MarchingCubes_Execute, STATGROUP_MarchingCubes);
static const int marchLookUp[4096] = { 0, 8, 3, 0, 1, 9, 1, 8, 3, 9, 8, 1, 1, 2, 10, 0, 8, 3, 1, 2, 10, 9, 2, 10, 0, 2, 9, 2, 8, 3, 2, 10, 8, 10, 9, 8, 3, 11, 2, 0, 11, 2, 8, 11, 0, 1, 9, 0, 2, 3, 11, 1, 11, 2, 1, 9, 11, 9, 8, 11, 3, 10, 1, 11, 10, 3, 0, 10, 1, 0, 8, 10, 8, 11, 10, 3, 9, 0, 3, 11, 9, 11, 10, 9, 9, 8, 10, 10, 8, 11, 4, 7, 8, 4, 3, 0, 7, 3, 4, 0, 1, 9, 8, 4, 7, 4, 1, 9, 4, 7, 1, 7, 3, 1, 1, 2, 10, 8, 4, 7, 3, 4, 7, 3, 0, 4, 1, 2, 10, 9, 2, 10, 9, 0, 2, 8, 4, 7, 2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, 8, 4, 7, 3, 11, 2, 11, 4, 7, 11, 2, 4, 2, 0, 4, 9, 0, 1, 8, 4, 7, 2, 3, 11, 4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, 3, 10, 1, 3, 11, 10, 7, 8, 4, 1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, 4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, 4, 7, 11, 4, 11, 9, 9, 11, 10, 9, 5, 4, 9, 5, 4, 0, 8, 3, 0, 5, 4, 1, 5, 0, 8, 5, 4, 8, 3, 5, 3, 1, 5, 1, 2, 10, 9, 5, 4, 3, 0, 8, 1, 2, 10, 4, 9, 5, 5, 2, 10, 5, 4, 2, 4, 0, 2, 2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, 9, 5, 4, 2, 3, 11, 0, 11, 2, 0, 8, 11, 4, 9, 5, 0, 5, 4, 0, 1, 5, 2, 3, 11, 2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, 10, 3, 11, 10, 1, 3, 9, 5, 4, 4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, 5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, 5, 4, 8, 5, 8, 10, 10, 8, 11, 9, 7, 8, 5, 7, 9, 9, 3, 0, 9, 5, 3, 5, 7, 3, 0, 7, 8, 0, 1, 7, 1, 5, 7, 1, 5, 3, 3, 5, 7, 9, 7, 8, 9, 5, 7, 10, 1, 2, 10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, 8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, 2, 10, 5, 2, 5, 3, 3, 5, 7, 7, 9, 5, 7, 8, 9, 3, 11, 2, 9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, 2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, 11, 2, 1, 11, 1, 7, 7, 1, 5, 9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, 5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, 11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, 11, 10, 5, 7, 11, 5, 10, 6, 5, 0, 8, 3, 5, 10, 6, 9, 0, 1, 5, 10, 6, 1, 8, 3, 1, 9, 8, 5, 10, 6, 1, 6, 5, 2, 6, 1, 1, 6, 5, 1, 2, 6, 3, 0, 8, 9, 6, 5, 9, 0, 6, 0, 2, 6, 5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, 2, 3, 11, 10, 6, 5, 11, 0, 8, 11, 2, 0, 10, 6, 5, 0, 1, 9, 2, 3, 11, 5, 10, 6, 5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, 6, 3, 11, 6, 5, 3, 5, 1, 3, 0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, 3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, 6, 5, 9, 6, 9, 11, 11, 9, 8, 5, 10, 6, 4, 7, 8, 4, 3, 0, 4, 7, 3, 6, 5, 10, 1, 9, 0, 5, 10, 6, 8, 4, 7, 10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, 6, 1, 2, 6, 5, 1, 4, 7, 8, 1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, 8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, 7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, 3, 11, 2, 7, 8, 4, 10, 6, 5, 5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, 0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, 9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, 8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, 5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, 0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, 6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, 10, 4, 9, 6, 4, 10, 4, 10, 6, 4, 9, 10, 0, 8, 3, 10, 0, 1, 10, 6, 0, 6, 4, 0, 8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, 1, 4, 9, 1, 2, 4, 2, 6, 4, 3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, 0, 2, 4, 4, 2, 6, 8, 3, 2, 8, 2, 4, 4, 2, 6, 10, 4, 9, 10, 6, 4, 11, 2, 3, 0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, 3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, 6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, 9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, 8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, 3, 11, 6, 3, 6, 0, 0, 6, 4, 6, 4, 8, 11, 6, 8, 7, 10, 6, 7, 8, 10, 8, 9, 10, 0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, 10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, 10, 6, 7, 10, 7, 1, 1, 7, 3, 1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, 2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, 7, 8, 0, 7, 0, 6, 6, 0, 2, 7, 3, 2, 6, 7, 2, 2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, 2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, 1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, 11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, 8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, 0, 9, 1, 11, 6, 7, 7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, 7, 11, 6, 7, 6, 11, 3, 0, 8, 11, 7, 6, 0, 1, 9, 11, 7, 6, 8, 1, 9, 8, 3, 1, 11, 7, 6, 10, 1, 2, 6, 11, 7, 1, 2, 10, 3, 0, 8, 6, 11, 7, 2, 9, 0, 2, 10, 9, 6, 11, 7, 6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, 7, 2, 3, 6, 2, 7, 7, 0, 8, 7, 6, 0, 6, 2, 0, 2, 7, 6, 2, 3, 7, 0, 1, 9, 1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, 10, 7, 6, 10, 1, 7, 1, 3, 7, 10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, 0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, 7, 6, 10, 7, 10, 8, 8, 10, 9, 6, 8, 4, 11, 8, 6, 3, 6, 11, 3, 0, 6, 0, 4, 6, 8, 6, 11, 8, 4, 6, 9, 0, 1, 9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, 6, 8, 4, 6, 11, 8, 2, 10, 1, 1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, 4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, 10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, 8, 2, 3, 8, 4, 2, 4, 6, 2, 0, 4, 2, 4, 6, 2, 1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, 1, 9, 4, 1, 4, 2, 2, 4, 6, 8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, 10, 1, 0, 10, 0, 6, 6, 0, 4, 4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, 10, 9, 4, 6, 10, 4, 4, 9, 5, 7, 6, 11, 0, 8, 3, 4, 9, 5, 11, 7, 6, 5, 0, 1, 5, 4, 0, 7, 6, 11, 11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, 9, 5, 4, 10, 1, 2, 7, 6, 11, 6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, 7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, 3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, 7, 2, 3, 7, 6, 2, 5, 4, 9, 9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, 3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, 6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, 9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, 1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, 4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, 7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, 6, 9, 5, 6, 11, 9, 11, 8, 9, 3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, 0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, 6, 11, 3, 6, 3, 5, 5, 3, 1, 1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, 0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, 11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, 6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, 5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, 9, 5, 6, 9, 6, 0, 0, 6, 2, 1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, 1, 5, 6, 2, 1, 6, 1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, 10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, 0, 3, 8, 5, 6, 10, 10, 5, 6, 11, 5, 10, 7, 5, 11, 11, 5, 10, 11, 7, 5, 8, 3, 0, 5, 11, 7, 5, 10, 11, 1, 9, 0, 10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, 11, 1, 2, 11, 7, 1, 7, 5, 1, 0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, 9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, 7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, 2, 5, 10, 2, 3, 5, 3, 7, 5, 8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, 9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, 9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, 1, 3, 5, 3, 7, 5, 0, 8, 7, 0, 7, 1, 1, 7, 5, 9, 0, 3, 9, 3, 5, 5, 3, 7, 9, 8, 7, 5, 9, 7, 5, 8, 4, 5, 10, 8, 10, 11, 8, 5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, 0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, 10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, 2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, 0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, 0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, 9, 4, 5, 2, 11, 3, 2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, 5, 10, 2, 5, 2, 4, 4, 2, 0, 3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, 5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, 8, 4, 5, 8, 5, 3, 3, 5, 1, 0, 4, 5, 1, 0, 5, 8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, 9, 4, 5, 4, 11, 7, 4, 9, 11, 9, 10, 11, 0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, 1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, 3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, 4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, 9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, 11, 7, 4, 11, 4, 2, 2, 4, 0, 11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, 2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, 9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, 3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, 1, 10, 2, 8, 7, 4, 4, 9, 1, 4, 1, 7, 7, 1, 3, 4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, 4, 0, 3, 7, 4, 3, 4, 8, 7, 9, 10, 8, 10, 11, 8, 3, 0, 9, 3, 9, 11, 11, 9, 10, 0, 1, 10, 0, 10, 8, 8, 10, 11, 3, 1, 10, 11, 3, 10, 1, 2, 11, 1, 11, 9, 9, 11, 8, 3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, 0, 2, 11, 8, 0, 11, 3, 2, 11, 2, 3, 8, 2, 8, 10, 10, 8, 9, 9, 10, 2, 0, 9, 2, 2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, 1, 10, 2, 1, 3, 8, 9, 1, 8, 0, 9, 1, 0, 3, 8};

class FMarchingCubes : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMarchingCubes);
	SHADER_USE_PARAMETER_STRUCT(FMarchingCubes, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FVector3f, leafPosition)
		SHADER_PARAMETER(uint32, leafDepth)		
		SHADER_PARAMETER(uint32, nodeIndex)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, isoValues)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, marchLookUp)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FVector>, outVertices)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FVector>, outNormals)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<int>, outTris)
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
		OutEnvironment.SetDefine(TEXT("THREADS_X"), voxelsPerAxis);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), voxelsPerAxis);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), voxelsPerAxis);
	}
};

IMPLEMENT_GLOBAL_SHADER(FMarchingCubes, "/MyShadersShaders/MarchingCubes.usf", "MarchingCubes", SF_Compute);

void AddOctreeMarchingPass(FRDGBuilder& GraphBuilder, OctreeNode* node, uint32 depth, uint32* nodeIndex, FMarchingCubesDispatchParams& Params, FRDGBufferUAVRef OutTrisUAV, FRDGBufferUAVRef OutNormalsUAV, FRDGBufferUAVRef OutVerticiesUAV, FRDGBufferSRVRef InLookUpSRV) {

	if (!node) return;
	depth++;
	if (!(node->isLeaf)) {
		for (OctreeNode* child : node->children)
			AddOctreeMarchingPass(GraphBuilder, child, depth, nodeIndex, Params, OutTrisUAV, OutNormalsUAV, OutVerticiesUAV, InLookUpSRV);
		return;
	}

	const FRDGBufferRef isoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"), sizeof(float), isoCount, node->isoAveragedValues, isoCount * sizeof(float));
	FMarchingCubes::FParameters* PassParams = GraphBuilder.AllocParameters<FMarchingCubes::FParameters>();
	PassParams->leafPosition = node->bounds.Center();
	PassParams->leafDepth = depth;
	PassParams->nodeIndex = (*nodeIndex);
	PassParams->isoValues = GraphBuilder.CreateSRV(isoValuesBuffer);
	PassParams->marchLookUp = InLookUpSRV;
	PassParams->outVertices = OutVerticiesUAV;
	PassParams->outNormals = OutNormalsUAV;
	PassParams->outTris = OutTrisUAV;
	PassParams->voxelsPerAxis = voxelsPerAxis;
	PassParams->baseDepthScale = Params.Input.baseDepthScale;
	PassParams->isoLevel = Params.Input.isoLevel;

	const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FMarchingCubes> ComputeShader(ShaderMap);
	// UE_LOG(LogTemp, Warning, TEXT("Set node index for dispatch: %d"), PassParams->nodeIndex);

	auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(voxelsPerAxis, voxelsPerAxis, voxelsPerAxis), FIntVector(1, 1, 1));

	GraphBuilder.AddPass(RDG_EVENT_NAME("Marching Cubes"), PassParams, ERDGPassFlags::AsyncCompute,
		[PassParams, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList) {
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParams, GroupCount); }
	);

	(*nodeIndex)++;
}

void FMarchingCubesInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FMarchingCubesDispatchParams Params, TFunction<void(FMarchingCubesOutput OutputVal)> AsyncCallback) {
	if (Params.Input.leafCount == 0) return;

	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_MarchingCubes_Execute);
		DECLARE_GPU_STAT(MarchingCubes)
		RDG_EVENT_SCOPE(GraphBuilder, "MarchingCubes");
		RDG_GPU_STAT_SCOPE(GraphBuilder, MarchingCubes);

		typename FMarchingCubes::FPermutationDomain PermutationVector;
		TShaderMapRef<FMarchingCubes> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			const int maxVoxelCount = nodeVoxelCount * Params.Input.leafCount;
			const int vertexCount = maxVoxelCount * 15;
			const int triCount = maxVoxelCount * 15; 

			TArray<FVector3f> OutVertices;
			TArray<FVector3f> OutNormals;
			TArray<int32> OutTris;

			OutVertices.Init(FVector3f(-1, -1, -1), vertexCount);
			OutNormals.Init(FVector3f(-1, -1, -1), vertexCount);
			OutTris.Init(-1, triCount);

			FRDGBufferRef isoValuesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("IsoValues_SB"), sizeof(int), 4096, marchLookUp, 4096 * sizeof(int));
			FRDGBufferRef OutVerticesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("OutVertices_StructuredBuffer"), sizeof(FVector3f), vertexCount, OutVertices.GetData(), vertexCount * sizeof(FVector3f));
			FRDGBufferRef OutNormalsBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("OutNormals_StructuredBuffer"), sizeof(FVector3f), vertexCount, OutNormals.GetData(), vertexCount * sizeof(FVector3f));
			FRDGBufferRef OutTrisBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("OutTris_StructuredBuffer"), sizeof(int32), triCount, OutTris.GetData(), triCount * sizeof(int32));

			FRDGBufferSRVRef InLookUpSRV = GraphBuilder.CreateSRV(isoValuesBuffer);
			FRDGBufferUAVRef OutTrisUAV = GraphBuilder.CreateUAV(OutTrisBuffer);
			FRDGBufferUAVRef OutNormalsUAV = GraphBuilder.CreateUAV(OutNormalsBuffer);
			FRDGBufferUAVRef OutVerticiesUAV = GraphBuilder.CreateUAV(OutVerticesBuffer);

			uint32 nodeIndex = 0;
			AddOctreeMarchingPass(GraphBuilder, Params.Input.tree, 0, &nodeIndex, Params, OutTrisUAV, OutNormalsUAV, OutVerticiesUAV, InLookUpSRV);

			FRHIGPUBufferReadback* VerticesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesVertices"));
			FRHIGPUBufferReadback* TrianglesReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesIndices"));
			FRHIGPUBufferReadback* NormalsReadback = new FRHIGPUBufferReadback(TEXT("MarchingCubesNormals"));

			AddEnqueueCopyPass(GraphBuilder, NormalsReadback, OutNormalsBuffer, 0u);
			AddEnqueueCopyPass(GraphBuilder, VerticesReadback, OutVerticesBuffer, 0u);
			AddEnqueueCopyPass(GraphBuilder, TrianglesReadback, OutTrisBuffer, 0u);

			auto RunnerFunc = [VerticesReadback, TrianglesReadback, NormalsReadback, AsyncCallback, vertexCount, triCount](auto&& RunnerFunc) ->
				void {
				if (VerticesReadback->IsReady() && TrianglesReadback->IsReady() && NormalsReadback->IsReady()) {
					FMarchingCubesOutput OutVal;

					void* VBuf = VerticesReadback->Lock(0);
					OutVal.outVertices.Append((FVector3f*)VBuf, vertexCount);
					VerticesReadback->Unlock();

					void* IBuf = TrianglesReadback->Lock(0);
					OutVal.outTris.Append((int32*)IBuf, triCount);
					TrianglesReadback->Unlock();

					void* NBuf = NormalsReadback->Lock(0);
					OutVal.outNormals.Append((FVector3f*)NBuf, vertexCount);
					NormalsReadback->Unlock();

					AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutVal]() {AsyncCallback(OutVal); });

					delete VerticesReadback;
					delete TrianglesReadback;
					delete NormalsReadback;
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