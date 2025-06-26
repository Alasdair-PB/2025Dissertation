#pragma once
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "ShaderParameterStruct.h"
#include "MeshMaterialShader.h"

class FVoxelPixelMeshMaterialShader : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FVoxelPixelMeshMaterialShader, MeshMaterial);

public:
	FVoxelPixelMeshMaterialShader() {}
	FVoxelPixelMeshMaterialShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer) {
	}

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return true;
	}

	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View) {}
};