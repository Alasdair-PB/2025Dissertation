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
	SHADER_PARAMETER_SRV(Buffer<float>, VertexFetch_Buffer)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
typedef TUniformBufferRef<FVoxelVertexFactoryParameters> FVoxelVertexFactoryBufferRef;

class FVoxelIndexBuffer : public FIndexBuffer
{
public:
	FVoxelIndexBuffer() = default;
	~FVoxelIndexBuffer() = default;
	FVoxelIndexBuffer(uint32 InNumIndices) : numIndices(InNumIndices) {}

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
	void SetElementCount(uint32 InNumIndices) { numIndices = InNumIndices; }

private:
	uint32 numIndices = 0;
};

class FVoxelVertexBuffer : public FVertexBuffer
{
public:
	FVoxelVertexBuffer() = default;
	~FVoxelVertexBuffer() = default;
	FVoxelVertexBuffer(uint32 InNumVertices) : numVertices(InNumVertices) {}

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
	void SetElementCount(uint32 InNumVertices) { numVertices = InNumVertices; }
private:
	uint32 numVertices = 0;
};

class FVoxelVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelVertexFactory);
public:

	FVoxelVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, uint32 bufferSize) : FVertexFactory(InFeatureLevel) //, FVoxelVertexFactoryParameters UniformParams
	{
		vertexBuffer.SetElementCount(bufferSize);
		indexBuffer.SetElementCount(bufferSize);
	}

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
	static void GetPSOPrecacheVertexFetchElements(EVertexInputStreamType VertexInputStreamType, FVertexDeclarationElementList& Elements);

	void InitRHI(FRHICommandListBase& RHICmdList) override final;
	void ReleaseRHI() override;

	FBufferRHIRef GetVertexBufferRHIRef() const { return vertexBuffer.GetRHI();}
	FBufferRHIRef GetIndexBufferRHIRef() const { return indexBuffer.GetRHI();}

	uint32 GetVertexBufferBytesPerElement() const { return sizeof(FVoxelVertexInfo); }
	uint32 GetIndexBufferBytesPerElement() const { return sizeof(uint32); }

	uint32 GetVertexBufferElementsCount() const { return vertexBuffer.GetVertexCount(); }
	uint32 GetIndexBufferElementsCount() const { return indexBuffer.GetIndexCount(); }

	static bool SupportsManualVertexFetch() { return true; }
	FVoxelVertexBuffer* GetVertexBuffer() { return &vertexBuffer; }
	FVoxelIndexBuffer* GetIndexBuffer() { return &indexBuffer; }
private:
	FVoxelVertexFactoryParameters params;
	FVoxelVertexFactoryBufferRef uniformBuffer;

	FVoxelIndexBuffer indexBuffer;
	FVoxelVertexBuffer vertexBuffer;

	uint32 firstIndex;
	bool bUsesDynamicParameter;
	friend class FVoxelVertexFactoryShaderParameters;
};