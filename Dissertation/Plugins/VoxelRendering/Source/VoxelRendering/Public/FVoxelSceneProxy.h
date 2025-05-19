#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "FVoxelVertexFactory.h"

class FVoxelSceneProxy : public FPrimitiveSceneProxy {
public:

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{

	}
};
