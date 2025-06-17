#pragma once
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"

class FVoxelPixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FVoxelPixelShader, Global);

public:
	FVoxelPixelShader() {}
	FVoxelPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer) {}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	void SetParameters(FRHICommandList& RHICmdList){}
};
