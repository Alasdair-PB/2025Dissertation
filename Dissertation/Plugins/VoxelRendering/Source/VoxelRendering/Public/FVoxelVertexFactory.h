#pragma once
#include "CoreMinimal.h"
#include "VertexFactory.h"
#include "UniformBuffer.h"
#include "MeshMaterialShader.h"
#include "ShaderParameters.h"
#include "RHI.h"
#include "RHIUtilities.h"
#include "FVoxelSceneProxy.h"
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
#include "VoxelBufferUtils.h"

class FVoxelVertexFactoryShaderParameters;
struct FShaderCompilerEnvironment;

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FVoxelVertexFactoryParameters, )
	SHADER_PARAMETER_SRV(Texture2D<float>, NoiseTexture)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
typedef TUniformBufferRef<FVoxelVertexFactoryParameters> FVoxelVertexFactoryBufferRef;

class FVoxelIndexBuffer : public FIndexBuffer
{
public:
	FVoxelIndexBuffer() = default;
	~FVoxelIndexBuffer() = default;
	FVoxelIndexBuffer(int32 InNumIndices) : numIndices(InNumIndices) {}

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
	{
		uint32 Size = sizeof(uint32) * numIndices;
		FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelIndexBuffer"));
		EBufferUsageFlags UsageFlags = BUF_UnorderedAccess | BUF_ShaderResource | BUF_IndexBuffer;
		const ERHIAccess InitialState = ERHIAccess::UAVCompute | ERHIAccess::SRVCompute;

		IndexBufferRHI = RHICmdList.CreateBuffer(Size, UsageFlags, 0, InitialState, CreateInfo);
		void* LockedData = RHICmdList.LockBuffer(IndexBufferRHI, 0, Size, RLM_WriteOnly);
		FMemory::Memzero(LockedData, Size); // FMemory::Memset(LockedData, 0xFF, Size);
		RHICmdList.UnlockBuffer(IndexBufferRHI);
	}
	uint32 GetIndexCount() const { return numIndices; }

private:
	int32 numIndices = 0;
};

class FVoxelVertexBuffer : public FVertexBuffer
{
public:
	FVoxelVertexBuffer() = default;
	~FVoxelVertexBuffer() = default;
	FVoxelVertexBuffer(int32 InNumVertices) : numVertices(InNumVertices) {}

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
	{
		uint32 Size = sizeof(FVoxelVertexInfo) * numVertices;
		FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelVertexBuffer"));
		EBufferUsageFlags UsageFlags = BUF_UnorderedAccess | BUF_ShaderResource | BUF_VertexBuffer;
		const ERHIAccess InitialState = ERHIAccess::UAVCompute | ERHIAccess::SRVCompute;

		VertexBufferRHI = RHICmdList.CreateBuffer(Size, UsageFlags, 0, InitialState, CreateInfo);
		void* LockedData = RHICmdList.LockBuffer(VertexBufferRHI, 0, Size, RLM_WriteOnly);
		FMemory::Memzero(LockedData, Size);		//FMemory::Memcpy(LockedData, Vertices.GetData(), Size);
		RHICmdList.UnlockBuffer(VertexBufferRHI);
	}
	uint32 GetVertexCount() const { return numVertices;}
private:
	int32 numVertices = 0;
};

class FVoxelVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelVertexFactory);
public:

	FVoxelVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, FVoxelVertexFactoryParameters UniformParams) : FVertexFactory(InFeatureLevel) //, FVoxelVertexFactoryParameters UniformParams
	{

	}

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
	static void GetPSOPrecacheVertexFetchElements(EVertexInputStreamType VertexInputStreamType, FVertexDeclarationElementList& Elements);

	void InitRHI(FRHICommandListBase& RHICmdList) override final;
	void ReleaseRHI() override;

	void SetVertexBuffer(const FVertexBuffer* InBuffer, uint32 StreamOffset, uint32 Stride);
	ENGINE_API void SetDynamicParameterBuffer(const FVertexBuffer* InDynamicParameterBuffer, uint32 StreamOffset, uint32 Stride);

	inline void SetUsesDynamicParameter(bool bInUsesDynamicParameter) { bUsesDynamicParameter = bInUsesDynamicParameter;}

	FBufferRHIRef GetVertexBufferRHIRef() const { return vertexBuffer.GetRHI();}
	FBufferRHIRef GetIndexBufferRHIRef() const { return indexBuffer.GetRHI();}

	uint32 GetVertexBufferBytesPerElement() const { return sizeof(FVoxelVertexInfo); }
	uint32 GetIndexBufferBytesPerElement() const { return sizeof(uint32); }

	uint32 GetVertexBufferElementsCount() const { return vertexBuffer.GetVertexCount(); }
	uint32 GetIndexBufferElementsCount() const { return indexBuffer.GetIndexCount(); }

	FRHIUnorderedAccessView* GetVertexBufferUAV(FRHICommandListBase& RHICmdList) const
	{
		return RHICmdList.CreateUnorderedAccessView(
			vertexBuffer.VertexBufferRHI,
			sizeof(FVoxelVertexInfo),
			PF_Unknown
		);
	}

	FRHIUnorderedAccessView* GetIndexBufferUAV(FRHICommandListBase& RHICmdList) const
	{
		return RHICmdList.CreateUnorderedAccessView(
			indexBuffer.IndexBufferRHI,
			PF_R32_UINT
		);
	}

	FVoxelVertexBuffer const* GetVertexBuffer() const { return &vertexBuffer; }
	FVoxelIndexBuffer const* GetIndexBuffer() const { return &indexBuffer; }
private:
	FVoxelVertexFactoryParameters params;
	FVoxelVertexFactoryBufferRef uniformBuffer;

	FVoxelIndexBuffer indexBuffer;
	FVoxelVertexBuffer vertexBuffer;

	uint32 firstIndex;
	bool bUsesDynamicParameter;
	friend class FVoxelVertexFactoryShaderParameters;
};