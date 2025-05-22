#pragma once
#include "CoreMinimal.h"
#include "VertexFactory.h"
#include "UniformBuffer.h"
#include "MeshMaterialShader.h"
#include "ShaderParameters.h"
#include "RHIUtilities.h"
#include "FVoxelSceneProxy.h"

class FVoxelVertexFactoryShaderParameters;
struct FShaderCompilerEnvironment;

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FVoxelVertexFactoryParameters, )
	SHADER_PARAMETER_SRV(Texture2D<float>, NoiseTexture)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
typedef TUniformBufferRef<FVoxelVertexFactoryParameters> FVoxelVertexFactoryBufferRef;

struct FVoxelVertexInfo
{
	FVoxelVertexInfo() {}
	FVoxelVertexInfo(const FVector& InPosition, const FVector& InNormal, const FColor& InColor) :
		Position(InPosition),
		Color(InColor)
	{}

	FVector Position;
	FVector Normal;
	FColor Color;
};

class FVoxelIndexBuffer : public FIndexBuffer
{
public:
	FVoxelIndexBuffer() = default;
	~FVoxelIndexBuffer() = default;

	int32 NumIndices = 0;

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
	{
		uint32 Size = sizeof(uint32) * NumIndices;
		FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelIndexBuffer"));

		IndexBufferRHI = RHICmdList.CreateBuffer(
			Size,
			BUF_UnorderedAccess | BUF_ShaderResource | BUF_IndexBuffer,
			0,
			ERHIAccess::UAVCompute | ERHIAccess::SRVCompute,
			CreateInfo
		);

		void* LockedData = RHICmdList.LockBuffer(IndexBufferRHI, 0, Size, RLM_WriteOnly);
		FMemory::Memzero(LockedData, Size);
		RHICmdList.UnlockBuffer(IndexBufferRHI);
	}

};


class FVoxelVertexBuffer : public FVertexBuffer
{
public:
	FVoxelVertexBuffer() = default;
	~FVoxelVertexBuffer() = default;
	int32 NumVertices = 0;

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
	{
		uint32 Size = sizeof(FVoxelVertexInfo) * NumVertices;
		FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelVertexBuffer"));

		VertexBufferRHI = RHICmdList.CreateBuffer(
			Size,
			BUF_UnorderedAccess | BUF_ShaderResource | BUF_VertexBuffer,
			0,
			ERHIAccess::UAVCompute | ERHIAccess::SRVCompute,
			CreateInfo
		);

		void* LockedData = RHICmdList.LockBuffer(VertexBufferRHI, 0, Size, RLM_WriteOnly);
		FMemory::Memzero(LockedData, Size);		//FMemory::Memcpy(LockedData, Vertices.GetData(), Size);
		RHICmdList.UnlockBuffer(VertexBufferRHI);
	}
};

class FVoxelVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelVertexFactory);
public:

	FVoxelVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, FVoxelVertexFactoryParameters UniformParams) : FVertexFactory(InFeatureLevel)
	{

	}

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
	static void GetPSOPrecacheVertexFetchElements(EVertexInputStreamType VertexInputStreamType, FVertexDeclarationElementList& Elements);

	void InitRHI(FRHICommandListBase& RHICmdList) override final;
	void ReleaseRHI() override;

	void SetVertexBuffer(const FVertexBuffer* InBuffer, uint32 StreamOffset, uint32 Stride);
	ENGINE_API void SetDynamicParameterBuffer(const FVertexBuffer* InDynamicParameterBuffer, uint32 StreamOffset, uint32 Stride);

	inline void SetUsesDynamicParameter(bool bInUsesDynamicParameter)
	{
		bUsesDynamicParameter = bInUsesDynamicParameter;
	}

	FRHIUnorderedAccessView* GetVertexBufferUAV(FRHICommandListBase& RHICmdList) const
	{
		return RHICmdList.CreateUnorderedAccessView(
			GVoxelVertexBuffer.VertexBufferRHI,
			sizeof(FVoxelVertexInfo),
			PF_Unknown
		);
	}

	FRHIUnorderedAccessView* GetIndexBufferUAV(FRHICommandListBase& RHICmdList) const
	{
		return RHICmdList.CreateUnorderedAccessView(
			GVoxelIndexBuffer.IndexBufferRHI,
			PF_R32_UINT
		);
	}

	FVoxelVertexBuffer const* GetVertexBuffer() const { return &GVoxelVertexBuffer; }
	FVoxelIndexBuffer const* GetIndexBuffer() const { return &GVoxelIndexBuffer; }

private:
	FVoxelVertexFactoryParameters Params;
	FVoxelVertexFactoryBufferRef UniformBuffer;
	TGlobalResource<FVoxelIndexBuffer, FRenderResource::EInitPhase::Pre> GVoxelIndexBuffer;
	TGlobalResource<FVoxelVertexBuffer, FRenderResource::EInitPhase::Pre> GVoxelVertexBuffer;
	uint32 FirstIndex;
	bool bUsesDynamicParameter;
	friend class FVoxelVertexFactoryShaderParameters;
};