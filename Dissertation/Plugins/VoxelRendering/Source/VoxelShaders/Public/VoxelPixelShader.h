#pragma once
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "ShaderParameterStruct.h"
#include "RHIStaticStates.h"

class FVoxelPixelShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FVoxelPixelShader);
	SHADER_USE_PARAMETER_STRUCT(FVoxelPixelShader, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	void SetParameters(FRHICommandList& RHICmdList){}
};
