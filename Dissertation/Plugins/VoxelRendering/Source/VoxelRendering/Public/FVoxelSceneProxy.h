#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "PrimitiveSceneProxy.h"
#include "FVoxelVertexFactory.h"
#include "Materials/MaterialRelevance.h"
class FVoxelVertexFactory;

class FVoxelSceneProxy : public FPrimitiveSceneProxy {
public:
	FVoxelSceneProxy(UPrimitiveComponent* Component);
	~FVoxelSceneProxy();
	FORCENOINLINE virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

	virtual SIZE_T GetTypeHash() const override {
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	virtual uint32 GetMemoryFootprint() const override { return(sizeof(*this) + GetAllocatedSize()); }
	virtual void CreateRenderThreadResources(FRHICommandListBase& RHICmdList) override;
	virtual void DestroyRenderThreadResources() override;
	virtual void OnTransformChanged(FRHICommandListBase& RHICmdList) override;
	void RenderMyCustomPass(FRHICommandListImmediate& RHICmdList, const FScene* Scene, const FSceneView* View, FTextureRHIRef Target);

	FVoxelVertexFactory* GetVertexFactory();
	UMaterialInterface* Material;

protected:
	bool bCompatiblePlatform;
	bool CanBeRendered() const { return bCompatiblePlatform; }
	FVoxelVertexFactory* VertexFactory;

	void SetMeshBatchGeneric(FMeshBatch& meshBatch, int32 viewIndex, bool bWireframe = false) const;
	void SetMeshBatchElementsGeneric(FMeshBatch& meshBatch, int32 viewIndex) const;
	void SetMeshBatchRenderProxy(FMeshBatch& meshBatch, int32 viewIndex, bool bWireframe) const;
	void SetMeshBatchElementsUserData(FMeshBatchElement& meshBatch) const;

	mutable TArray<FMeshBatch> CustomPassMeshBatches;
};
