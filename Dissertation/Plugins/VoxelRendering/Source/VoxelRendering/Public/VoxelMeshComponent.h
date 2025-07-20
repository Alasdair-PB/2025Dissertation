#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "FVoxelSceneProxy.h"
#include "Octree.h"
#include "OctreeNode.h"
#include "MarchingCubesDispatcher.h"
#include "VoxelSceneViewExtension.h"
#include "VoxelMeshComponent.generated.h"

static const float isoLevel = 0.5f;
class FPrimitiveSceneProxy;

UCLASS()
class VOXELRENDERING_API UVoxelMeshComponent : public UMeshComponent
{
    GENERATED_BODY()

public:
    UVoxelMeshComponent();
    void InitVoxelMesh(float scale, int inBufferSizePerAxis, int depth, int voxelsPerAxis, TArray<float>& in_isoValueBuffer, TArray<uint32>& in_typeValueBuffer,
        AActor* inEraser, AActor* inPlayer)
    ;
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
    APlayerController* playerController;
    AActor* eraser;
    AActor* player;
    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;
    virtual void OnRegister() override;
    void GetVisibleNodes(TArray<OctreeNode*>& nodes, OctreeNode* node);
    void InvokeVoxelRenderPasses();
    void CheckVoxelMining();
    void RotateAroundAxis(FVector axis, float degreeTick);
    void SetRenderDataLOD();
    void InvokeVoxelRenderer(TArray<FVoxelComputeUpdateNodeData>& updateData);
    void TraverseAndDraw(OctreeNode* node);
    float SampleSDF(FVector3f p);

    FVoxelSceneProxy* sceneProxy;
    Octree* tree;
    UBodySetup* voxelBodySetup;
};