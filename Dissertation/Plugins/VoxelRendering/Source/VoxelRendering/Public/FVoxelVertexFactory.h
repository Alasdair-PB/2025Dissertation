#pragma once
#include "CoreMinimal.h"
#include "VertexFactory.h"
#include "UniformBuffer.h"
#include "MeshMaterialShader.h"
#include "ShaderParameters.h"
#include "RHIUtilities.h"
#include "FVoxelSceneProxy.h"

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

class FVoxelVertexBuffer : public FVertexBuffer
{
public:
	FVoxelVertexBuffer() = default;
	~FVoxelVertexBuffer() = default;

	TArray<FVoxelVertexInfo> Vertices;

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
	{
		uint32 Size = sizeof(FVoxelVertexInfo) * Vertices.Num();
		FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelVertexBuffer"));

		VertexBufferRHI = RHICmdList.CreateBuffer(Size, BUF_Static | BUF_VertexBuffer | BUF_ShaderResource, 0, ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask, CreateInfo);
		void* LockedData = RHICmdList.LockBuffer(VertexBufferRHI, 0, Size, RLM_WriteOnly);
		FMemory::Memzero(LockedData, Size);
		RHICmdList.UnlockBuffer(VertexBufferRHI);
	}
};

class FVoxelVertexFactory : FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelVertexFactory);
public:

	FVoxelVertexFactory(ERHIFeatureLevel::Type InFeatureLevel) : FVertexFactory(InFeatureLevel){}

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

	FIndexBuffer*& GetIndexBuffer()
	{
		return IndexBuffer;
	}

private:
	FIndexBuffer* IndexBuffer;
	TGlobalResource<FVoxelVertexBuffer, FRenderResource::EInitPhase::Pre> GVoxelVertexBuffer;
	FVoxelSceneProxy* SceneProxy;
	uint32 FirstIndex;
	int32 OutTriangleCount;
	bool bUsesDynamicParameter;
};