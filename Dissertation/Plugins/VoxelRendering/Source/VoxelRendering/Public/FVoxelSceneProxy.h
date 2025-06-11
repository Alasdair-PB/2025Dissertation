#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "PrimitiveSceneProxy.h"
#include "FVoxelVertexFactory.h"
#include "Materials/MaterialRelevance.h"
class FVoxelVertexFactory;

class FVoxelSceneProxy : public FPrimitiveSceneProxy {
public:
	FVoxelSceneProxy(UPrimitiveComponent* Component) : 
		FPrimitiveSceneProxy(Component),
		bCompatiblePlatform(GetScene().GetFeatureLevel() >= ERHIFeatureLevel::SM5)
	{}

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
	//virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;

	FVoxelVertexFactory* GetVertexFactor();
	UMaterialInterface* Material;

protected:
	bool bCompatiblePlatform;
	bool CanBeRendered() const { return bCompatiblePlatform; }
	FVoxelVertexFactory* VertexFactory;
	void DrawDynamicElements(FMeshBatch& Mesh, FMaterialRenderProxy* MaterialProxy, bool bWireframe, int32 ViewIndex) const;

};
