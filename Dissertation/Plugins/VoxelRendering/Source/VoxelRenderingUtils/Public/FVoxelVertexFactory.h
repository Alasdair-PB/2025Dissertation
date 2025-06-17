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
#include "FVoxelVertexFactoryShaderParameters.h"

struct FShaderCompilerEnvironment;

/*
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FVoxelComputeFactoryUniformParameters, )
	//SHADER_PARAMETER_UAV(RWStructuredBuffer<FVoxelVertexInfo>, VertexFetch_Buffer)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
typedef TUniformBufferRef<FVoxelComputeFactoryUniformParameters> FVoxelComputeFactoryBufferRef;
*/
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FVoxelVertexFactoryUniformParameters, )
	//SHADER_PARAMETER_SRV(Buffer<float>, VertexFetch_Buffer)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
typedef TUniformBufferRef<FVoxelVertexFactoryUniformParameters> FVoxelVertexFactoryBufferRef;


struct VOXELRENDERINGUTILS_API FVoxelBatchElementUserData
{
	//int32 voxelsPerAxis;
	//int32 baseDepthScale;
	//int32 isoLevel;
	FVoxelBatchElementUserData();
};
class VOXELRENDERINGUTILS_API FVoxelIndexBuffer : public FIndexBuffer
{
public:
	FVoxelIndexBuffer() = default;
	~FVoxelIndexBuffer() = default;
	FVoxelIndexBuffer(uint32 InNumIndices) : numIndices(InNumIndices) {}

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
	uint32 GetIndexCount() const { return numIndices; }
	void SetElementCount(uint32 InNumIndices) { numIndices = InNumIndices; }
	FShaderResourceViewRHIRef SRV;

private:
	uint32 numIndices = 0;
};

class VOXELRENDERINGUTILS_API FVoxelVertexBuffer : public FVertexBuffer
{
public:
	FVoxelVertexBuffer() = default;
	~FVoxelVertexBuffer() = default;
	FVoxelVertexBuffer(uint32 InNumVertices) : numVertices(InNumVertices) {}

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override;

	uint32 GetVertexCount() const { return numVertices;}
	void SetElementCount(uint32 InNumVertices) { numVertices = InNumVertices; }	
	FShaderResourceViewRHIRef SRV;
	FUnorderedAccessViewRHIRef UAV;

private:
	uint32 numVertices = 0;
	//FVoxelVertexFactoryBufferRef VertexUniformBuffer;
	//FVoxelComputeFactoryBufferRef ComputeUniformBuffer;
};

class VOXELRENDERINGUTILS_API FVoxelVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelVertexFactory);

public:
	FVoxelVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, uint32 bufferSize);

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
	static void GetPSOPrecacheVertexFetchElements(EVertexInputStreamType VertexInputStreamType, FVertexDeclarationElementList& Elements);

	void SetUniformParameters();
	void InitRHI(FRHICommandListBase& RHICmdList) override final;
	void ReleaseRHI() override;

	FBufferRHIRef GetVertexBufferRHIRef() const { return vertexBuffer.GetRHI();}
	FBufferRHIRef GetIndexBufferRHIRef() const { return indexBuffer.GetRHI();}

	FShaderResourceViewRHIRef GetVertexSRV() const { return vertexBuffer.SRV; }
	FUnorderedAccessViewRHIRef GetVertexUAV() const { return vertexBuffer.UAV; }

	FShaderResourceViewRHIRef GetIndexSRV() const { return indexBuffer.SRV; }


	uint32 GetVertexBufferBytesPerElement() const { return sizeof(FVoxelVertexInfo); }
	uint32 GetIndexBufferBytesPerElement() const { return sizeof(uint32); }

	uint32 GetVertexBufferElementsCount() const { return vertexBuffer.GetVertexCount(); }
	uint32 GetIndexBufferElementsCount() const { return indexBuffer.GetIndexCount(); }

	bool SupportsManualVertexFetch() { return true; }
	FVoxelVertexBuffer* GetVertexBuffer() { return &vertexBuffer; }
	FVoxelIndexBuffer* GetIndexBuffer() { return &indexBuffer; }
private:
	FVoxelIndexBuffer indexBuffer;
	FVoxelVertexBuffer vertexBuffer;

	//FVoxelComputeFactoryBufferRef computeUniformBuffer;
	FVoxelVertexFactoryBufferRef vertexUniformBuffer;

	uint32 firstIndex;
	bool bUsesDynamicParameter;
	friend class FVoxelVertexFactoryShaderParameters;
};