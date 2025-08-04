#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "FVoxelSceneProxy.h"
#include "Octree.h"
#include "OctreeNode.h"
#include "MarchingCubesDispatcher.h"
#include "VoxelSceneViewExtension.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "VoxelMeshComponent.generated.h"

static const float isoLevel = 0.5f;
class FPrimitiveSceneProxy;

class Palette {
public:
    Palette(float inMaxRadius, float inMaxPower, int inMaxType) : 
        brushRadius(30.0f), maxRadius(inMaxRadius), 
        brushPower(0.5f), maxBrushPower(inMaxPower), 
        paintType(1), maxTypes(inMaxType) {}

    void SetBrushRadius(float inRadius) { brushRadius = FMath::Clamp(inRadius, 0.0f, maxRadius); }
    void SetPaintType(int inType) { paintType = FMath::Clamp(inType, 0.0f, maxTypes); }
    void SetBrushPower(float inPower) { brushPower = FMath::Clamp(inPower, 0.0f, maxBrushPower); }

    float GetBrushPower() const { return brushPower; }
    float GetBrushRadius() const { return brushRadius; }
    int GetPaintType() const { return paintType; }

protected:
    float brushRadius;
    float maxRadius; 
    float brushPower;
    float maxBrushPower;
    int paintType;
    int maxTypes;
};


UCLASS()
class VOXELRENDERING_API UVoxelMeshComponent : public UMeshComponent
{
    GENERATED_BODY()

public:
    UVoxelMeshComponent();
    void InitVoxelMesh(float scale, int inBufferSizePerAxis, int depth, int voxelsPerAxis, TArray<float>& in_isoValueBuffer, TArray<uint32>& in_typeValueBuffer,
        AActor* inEraser, AActor* inPlayer, UNiagaraSystem* vfxSystem)
    ;
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual int32 GetNumMaterials() const override { return 1; }

    virtual void PostLoad() override;
    virtual UBodySetup* GetBodySetup() override;
    virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial) override;
    virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override { return Material; }

    void EnableRotation() { rotatePlanet = true; }
    void DisableRotation() { rotatePlanet = false; }

    void TogglePlanetRotate() { rotatePlanet = !rotatePlanet; }
    void ToggleDebugNode() { debugNodes = !debugNodes; }
    void ToggleDeform() { deform = !deform; }

    void SetRotationState(bool inState) { rotatePlanet = inState; }
    void SetDebugNodesState(bool inState) { debugNodes = inState; }
    void RefreshDeformation();

    void SetBrushDensity(float density) { if (palette) palette->SetBrushPower(density);}
    void SetBrushRadius(float radius) { if (palette) palette->SetBrushRadius(radius);}
    void SetPaintType(int type) { if (palette) palette->SetPaintType(type);}
    void ToggleLODState() { usePlayerLOD = !usePlayerLOD; }
    void UpdateSceneProxyNodes(const TArray<FVoxelProxyUpdateDataNode> updateNodes);

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
    void InvokeVoxelRenderer(TArray<FVoxelComputeUpdateNodeData>& updateData, TArray<FVoxelTransVoxelNodeData>& transVoxelUpdateData, const TArray<FVoxelProxyUpdateDataNode> updateNodes);
    void TraverseAndDraw();
    void SetNodeVisible(TArray<OctreeNode*>& nodes, OctreeNode* node);
    void SetChildrenVisible(TArray<OctreeNode*>& pushStack, OctreeNode* node, int currentDepth, int targetDepth);
    bool BalanceNode(TArray<OctreeNode*>& removalStack, TArray<OctreeNode*>& pushStack, TArray<OctreeNode*>& visibleNodes, OctreeNode* node);
    void BalanceVisibleNodes(TArray<OctreeNode*>&visibleNodes);
    void CheckRotation();
    void InitMaterial();

    Palette* palette;
    FVoxelSceneProxy* sceneProxy;
    Octree* tree;
    UBodySetup* voxelBodySetup;
    UNiagaraSystem* vfxSystem;
    bool rotatePlanet;
    bool debugNodes;
    bool usePlayerLOD;
    bool deform;
};