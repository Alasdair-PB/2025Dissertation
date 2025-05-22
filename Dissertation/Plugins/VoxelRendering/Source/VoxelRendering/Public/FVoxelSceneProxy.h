#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "PrimitiveSceneProxy.h"
#include "FVoxelSectionProxy.h"
#include "Materials/MaterialRelevance.h"

class FVoxelSceneProxy : public FPrimitiveSceneProxy {
public:
	FVoxelSceneProxy(UPrimitiveComponent* Component)
		: FPrimitiveSceneProxy(Component)
	{}

	FORCENOINLINE virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	void DrawDynamicElements(FMeshBatch& Mesh, FMaterialRenderProxy* MaterialProxy, bool bWireframe, int32 ViewIndex) const;
	void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI, int LODIndex);

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

	virtual SIZE_T GetTypeHash() const override {
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	virtual uint32 GetMemoryFootprint() const override { return(sizeof(*this) + GetAllocatedSize()); }
	virtual void CreateRenderThreadResources(FRHICommandListBase& RHICmdList) override;
	virtual void DestroyRenderThreadResources() override;
	virtual void OnTransformChanged(FRHICommandListBase& RHICmdList) override;

	class FVoxelVertexFactory* VertexFactory;
	UMaterialInterface* Material;

protected:
};
