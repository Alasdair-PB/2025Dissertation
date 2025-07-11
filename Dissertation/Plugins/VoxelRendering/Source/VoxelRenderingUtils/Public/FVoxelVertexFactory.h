#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "VertexFactory.h"
#include "UniformBuffer.h"
#include "MeshMaterialShader.h"
#include "ShaderParameters.h"
#include "RHI.h"
#include "RHIUtilities.h"
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
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "FVoxelVertexFactoryShaderParameters.h"

struct FShaderCompilerEnvironment;

struct VOXELRENDERINGUTILS_API FVoxelBatchElementUserData
{
	FVoxelBatchElementUserData();
};

class VOXELRENDERINGUTILS_API FVoxelIndexBuffer : public FIndexBuffer
{
public:
	FVoxelIndexBuffer() = default;
	~FVoxelIndexBuffer() = default;
	FVoxelIndexBuffer(uint32 InNumIndices) : numIndices(InNumIndices) {}

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
	virtual void ReleaseRHI() override;

	uint32 GetIndexCount() const { return numIndices; }
	uint32 GetVisibleIndiceCount() const { return visibleIndicies; }
	void SetVisibleIndiciesCount(uint32 inVisibleIndicies) { visibleIndicies = inVisibleIndicies; }

	void SetElementCount(uint32 inNumIndices) { 
		numIndices = inNumIndices; 
		visibleIndicies = inNumIndices;
	}

	FShaderResourceViewRHIRef SRV;
private:
	uint32 numIndices = 0;
	uint32 visibleIndicies = 0;
};

class VOXELRENDERINGUTILS_API FVoxelVertexBuffer : public FVertexBuffer
{
public:
	FVoxelVertexBuffer() = default;
	~FVoxelVertexBuffer() = default;
	FVoxelVertexBuffer(uint32 InNumVertices) : numVertices(InNumVertices) {}

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
	virtual void ReleaseRHI() override;
	uint32 GetVertexCount() const { return numVertices;}

	void SetElementCount(uint32 InNumVertices) { 
		numVertices = InNumVertices; 
		visibleVerticies = InNumVertices;
	}	

	uint32 GetVisibleVerticiesCount() const { return visibleVerticies; }
	void SetVisibleVerticiessCount(uint32 inVisibleVerticies) { visibleVerticies = inVisibleVerticies; }

	FShaderResourceViewRHIRef SRV;
	FUnorderedAccessViewRHIRef UAV;

private:
	uint32 numVertices = 0;
	uint32 visibleVerticies = 0;
};

class VOXELRENDERINGUTILS_API FVoxelVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelVertexFactory);

public:
	FVoxelVertexFactory(uint32 bufferSize);
	~FVoxelVertexFactory();
	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	void InitRHI(FRHICommandListBase& RHICmdList) override final;
	void ReleaseRHI() override;
	static bool ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType) { return true; }

	void Initialize(uint32 bufferSize);

	FBufferRHIRef GetVertexBufferRHIRef() const { return vertexBuffer.GetRHI();}
	FBufferRHIRef GetIndexBufferRHIRef() const { return indexBuffer.GetRHI();}

	FShaderResourceViewRHIRef GetVertexSRV() const { return vertexBuffer.SRV; }
	FUnorderedAccessViewRHIRef GetVertexUAV() const { return vertexBuffer.UAV; }
	FShaderResourceViewRHIRef GetVertexNormalsSRV() const { return normalsBuffer.SRV; }
	FUnorderedAccessViewRHIRef GetVertexNormalsUAV() const { return normalsBuffer.UAV; }
	FShaderResourceViewRHIRef GetIndexSRV() const { return indexBuffer.SRV; }

	uint32 GetVertexBufferBytesPerElement() const { return sizeof(FVoxelVertexInfo); }
	uint32 GetIndexBufferBytesPerElement() const { return sizeof(uint32); }

	uint32 GetVertexBufferElementsCount() const { return vertexBuffer.GetVertexCount(); }
	uint32 GetIndexBufferElementsCount() const { return indexBuffer.GetIndexCount(); }

	bool SupportsManualVertexFetch() { return true; }
	FVoxelVertexBuffer* GetVertexBuffer() { return &vertexBuffer; }
	FVoxelIndexBuffer* GetIndexBuffer() { return &indexBuffer; }
	FVoxelVertexBuffer* GetNormalBuffer() { return &normalsBuffer; }

	bool bInitialized = false;

private:
	FVoxelIndexBuffer indexBuffer;
	FVoxelVertexBuffer vertexBuffer;
	FVoxelVertexBuffer normalsBuffer;
	friend class FVoxelVertexFactoryShaderParameters;
};