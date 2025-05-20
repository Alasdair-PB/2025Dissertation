#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "FVoxelSectionProxy.h"

class FVoxelSceneProxy : public FPrimitiveSceneProxy {
public:

	FORCENOINLINE virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	void DrawDynamicElements(const FVoxelProxySection* Section, FMeshBatch& Mesh, FMaterialRenderProxy* MaterialProxy, bool bWireframe, int32 ViewIndex) const;
	void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI, FVoxelProxySection* Section, int LODIndex);

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

protected:
	FVertexFactory VertexFactory;
	FMaterialRelevance MaterialRelevance;
};
