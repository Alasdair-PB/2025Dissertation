#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "FVoxelSceneProxy.h"
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
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual int32 GetNumMaterials() const override { return 1; }

    virtual void PostLoad() override;
    virtual UBodySetup* GetBodySetup() override;
    virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial) override;

    virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override { return Material; }

private:
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> Material;
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
