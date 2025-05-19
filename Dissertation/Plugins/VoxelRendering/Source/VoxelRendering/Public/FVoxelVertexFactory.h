#pragma once
#include "CoreMinimal.h"
#include "VertexFactory.h"
#include "UniformBuffer.h"
#include "MeshMaterialShader.h"
#include "ShaderParameters.h"
#include "RHIUtilities.h"
#include "VertexFactory.h"
#include "FVoxelSceneProxy.h"
#include "FVoxelVertexFactoryShaderParameters.h"

class FVoxelVertexFactoryShaderParameters;

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
		
	struct DataType
	{
		FVertexStreamComponent PositionComponent;
		FVertexStreamComponent ColorComponent;
	};

	void InitRHI(FRHICommandListBase& RHICmdList) override final {

	}

	void ReleaseRHI() override {
		FVertexFactory::ReleaseRHI();
	}

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters) { return false; }
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {}
	static bool ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType) { return false; }

	inline void SetTransformIndex(uint16 Index) { TransformIndex = Index; }
	inline void SetSceneProxy(FVoxelSceneProxy* Proxy) { SceneProxy = Proxy; }

	void Init(const FVoxelVertexBuffer* VertexBuffer) {}
	void SetData(const DataType& InData) {}

	static FVoxelVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency);

private:
	DataType Data;

	uint16 TransformIndex;
	FVoxelSceneProxy* SceneProxy;
	friend class FVoxelVertexFactoryShaderParameters;
	TGlobalResource<FVoxelVertexBuffer, FRenderResource::EInitPhase::Pre> GVoxelVertexBuffer;
};