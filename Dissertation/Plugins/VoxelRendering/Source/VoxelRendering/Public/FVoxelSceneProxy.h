#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "PrimitiveSceneProxy.h"
#include "FVoxelVertexFactory.h"
#include "Materials/MaterialRelevance.h"
#include "RenderData.h"

class VOXELRENDERING_API FVoxelSceneProxy : public FPrimitiveSceneProxy {
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
	void UpdateSelectedNodes(const TArray<FVoxelProxyUpdateDataNode>& renderData);

	void RenderMyCustomPass(FRHICommandListImmediate& RHICmdList, const FScene* Scene, const FSceneView* View);
	bool IsInitialized();
	bool CanBeRendered() const { return bCompatiblePlatform; }

	UMaterialInterface* Material;
	TArray<FVoxelProxyUpdateDataNode> GetVisibleNodes() { return selectedNodes; }

protected:
	mutable TArray<FMeshBatch> CustomPassMeshBatches;
	TArray<FVoxelProxyUpdateDataNode> selectedNodes;
	bool bInitialized = false;
	bool bCompatiblePlatform = true;

	void SetMeshBatchRenderProxy(FMeshBatch& meshBatch) const;
	void SetMeshBatchGeneric(FMeshBatch& meshBatch, const FVoxelProxyUpdateDataNode& node, int32 viewIndex, bool bWireframe = false) const;
	void SetMeshBatchElementsGeneric(FMeshBatch& meshBatch, const FVoxelProxyUpdateDataNode& node, int32 viewIndex) const;
	void SetMeshBatchElementsUserData(FMeshBatchElement& meshBatch) const;

};
