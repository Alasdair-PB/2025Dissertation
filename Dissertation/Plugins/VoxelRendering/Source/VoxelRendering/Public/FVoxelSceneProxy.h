#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "PrimitiveSceneProxy.h"
#include "FVoxelSectionProxy.h"

class FVoxelSceneProxy : public FPrimitiveSceneProxy {
public:
	FVoxelSceneProxy():FPrimitiveSceneProxy(){}
	FORCENOINLINE virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	void DrawDynamicElements(const FVoxelProxySection* Section, FMeshBatch& Mesh, FMaterialRenderProxy* MaterialProxy, bool bWireframe, int32 ViewIndex) const;
	void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI, FVoxelProxySection* Section, int LODIndex);

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

	virtual SIZE_T GetTypeHash() const override {
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	virtual uint32 GetMemoryFootprint() const override { return(sizeof(*this) + GetAllocatedSize()); }

protected:
	FVertexFactory VertexFactory;
	FMaterialRelevance MaterialRelevance;
};
