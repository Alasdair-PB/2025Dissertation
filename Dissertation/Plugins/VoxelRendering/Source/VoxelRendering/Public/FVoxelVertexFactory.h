#pragma once
#include "CoreMinimal.h"
#include "VertexFactory.h"
#include "UniformBuffer.h"
#include "MeshMaterialShader.h"
#include "ShaderParameters.h"
#include "RHI.h"
#include "RHIUtilities.h"
#include "FVoxelSceneProxy.h"


#include "ComputeDispatchers.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
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
	FVoxelIndexBuffer(int32 InNumIndices) : NumIndices(InNumIndices) {}

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
	{
		uint32 Size = sizeof(uint32) * NumIndices;
		FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelIndexBuffer"));
		EBufferUsageFlags UsageFlags = BUF_UnorderedAccess | BUF_ShaderResource | BUF_IndexBuffer;
		const ERHIAccess InitialState = ERHIAccess::UAVCompute | ERHIAccess::SRVCompute;

		IndexBufferRHI = RHICmdList.CreateBuffer(Size, UsageFlags, 0, InitialState, CreateInfo);
		void* LockedData = RHICmdList.LockBuffer(IndexBufferRHI, 0, Size, RLM_WriteOnly);
		FMemory::Memzero(LockedData, Size);
		RHICmdList.UnlockBuffer(IndexBufferRHI);
	}
private:
	int32 NumIndices = 0;
};


class FVoxelVertexBuffer : public FVertexBuffer
{
public:
	FVoxelVertexBuffer() = default;
	~FVoxelVertexBuffer() = default;
	FVoxelVertexBuffer(int32 InNumVertices) : NumVertices(InNumVertices) {}

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
	{
		uint32 Size = sizeof(FVoxelVertexInfo) * NumVertices;
		FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelVertexBuffer"));
		EBufferUsageFlags UsageFlags = BUF_UnorderedAccess | BUF_ShaderResource | BUF_VertexBuffer;
		const ERHIAccess InitialState = ERHIAccess::UAVCompute | ERHIAccess::SRVCompute;

		VertexBufferRHI = RHICmdList.CreateBuffer(Size, UsageFlags, 0, InitialState, CreateInfo);
		void* LockedData = RHICmdList.LockBuffer(VertexBufferRHI, 0, Size, RLM_WriteOnly);
		FMemory::Memzero(LockedData, Size);		//FMemory::Memcpy(LockedData, Vertices.GetData(), Size);
		RHICmdList.UnlockBuffer(VertexBufferRHI);
	}

private:
	int32 NumVertices = 0;
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
			VertexBuffer.VertexBufferRHI,
			sizeof(FVoxelVertexInfo),
			PF_Unknown
		);
	}

	FRHIUnorderedAccessView* GetIndexBufferUAV(FRHICommandListBase& RHICmdList) const
	{
		return RHICmdList.CreateUnorderedAccessView(
			IndexBuffer.IndexBufferRHI,
			PF_R32_UINT
		);
	}

	FVoxelVertexBuffer const* GetVertexBuffer() const { return &VertexBuffer; }
	FVoxelIndexBuffer const* GetIndexBuffer() const { return &IndexBuffer; }
private:
	FVoxelVertexFactoryParameters Params;
	FVoxelVertexFactoryBufferRef UniformBuffer;

	FVoxelIndexBuffer IndexBuffer;
	FVoxelVertexBuffer VertexBuffer;

	uint32 FirstIndex;
	bool bUsesDynamicParameter;
	friend class FVoxelVertexFactoryShaderParameters;
};