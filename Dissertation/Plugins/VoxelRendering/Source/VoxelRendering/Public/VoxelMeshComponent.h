#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "FVoxelVertexFactory.h"
#include "Octree.h"
#include "OctreeNode.h"
#include "MarchingCubesDispatcher.h"
#include "VoxelMeshComponent.generated.h"

static const float isoLevel = 0.5f;
class FPrimitiveSceneProxy;

UCLASS()
class VOXELRENDERING_API UVoxelMeshComponent : public UMeshComponent
{
    GENERATED_BODY()

public:
    UVoxelMeshComponent();
    void InitVoxelMesh(AABB bounds, int depth, TArray<float>& in_isoValueBuffer, TArray<uint32>& in_typeValueBuffer);
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
protected:
    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;

    void InvokeVoxelRenderer(OctreeNode* node);
    void TraverseAndDraw(OctreeNode* node);
    float SampleSDF(FVector3f p);
    void BuildOctree(AABB bounds, int size, int depth);

    UMaterialInstanceDynamic* MaterialInstance;
    FVoxelSceneProxy* sceneProxy;
    Octree* tree;
    AABB bounds;

    TArray<float> isoValueBuffer;
    TArray<uint32> typeValueBuffer;

};
