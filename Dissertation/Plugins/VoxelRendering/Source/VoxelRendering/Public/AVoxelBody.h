#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Octree.h"
#include "Delegates/DelegateCombinations.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "AVoxelBody.generated.h"

class UVoxelMeshComponent;
class UBufferResource;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRefresh);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDebugToggle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRotateToggle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLODToggle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeformToggle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDensityDelta, float, value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRadiusDelta, float, value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTypeDelta, int, value);

UCLASS()
class VOXELRENDERING_API AVoxelBody : public AActor
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Events")
    static void BroadcastDensityDeltaEvent(float density);

    UFUNCTION(BlueprintCallable, Category = "Events")
    static void BroadcastRadiusDeltaEvent(float radius);

    UFUNCTION(BlueprintCallable, Category = "Events")
    static void BroadcastTypeDeltaEvent(int type);

    UFUNCTION(BlueprintCallable, Category = "Events")
    static void BroadcastRotateToggleEvent();

    UFUNCTION(BlueprintCallable, Category = "Events")
    static void BroadcastDeformToggleEvent();

    UFUNCTION(BlueprintCallable, Category = "Events")
    static void BroadcastDebugToggleEvent();

    UFUNCTION(BlueprintCallable, Category = "Events")
    static void BroadcastRefreshEvent();

    UFUNCTION(BlueprintCallable, Category = "Events")
    static void BroadcastLODToggleEvent();

    AVoxelBody();
    static AVoxelBody* CreateVoxelMeshActor(UWorld* World, float scale, int size, int depth, int voxelsPerAxis, 
        TArray<float>& inIsovalueBuffer, TArray<uint32>& inTypeValueBuffer, 
        AActor* eraser, AActor* player, UNiagaraSystem* vfxSystem);

    void SetMeshComponent(UVoxelMeshComponent* inMeshComponent);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleNodeDebug();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void RefreshDeformation();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void TogglePlanetRotate();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleDeform();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetBrushDensity(float density);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetBrushRadius(float radius);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetBrushType(int type);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleLOD();

    static FOnRefresh onRefresh;
    static FOnDebugToggle onDebugToggle;
    static FOnRotateToggle onRotateToggle;
    static FOnLODToggle onLODToggle;
    static FOnDeformToggle onDeformToggle;

    static FOnDensityDelta onDensityDelta;
    static FOnRadiusDelta onRadiusDelta;
    static FOnTypeDelta onTypeDelta;

    void BeginPlay()
    {
        Super::BeginPlay();
        onRefresh.AddDynamic(this, &AVoxelBody::RefreshDeformation);
        onDebugToggle.AddDynamic(this, &AVoxelBody::ToggleNodeDebug);
        onRotateToggle.AddDynamic(this, &AVoxelBody::TogglePlanetRotate);

        onDensityDelta.AddDynamic(this, &AVoxelBody::SetBrushDensity);
        onRadiusDelta.AddDynamic(this, &AVoxelBody::SetBrushRadius);
        onTypeDelta.AddDynamic(this, &AVoxelBody::SetBrushType);
        onLODToggle.AddDynamic(this, &AVoxelBody::ToggleLOD);
        onDeformToggle.AddDynamic(this, &AVoxelBody::ToggleDeform);
    }

    void EndPlay(const EEndPlayReason::Type EndPlayReason)
    {
        Super::EndPlay(EndPlayReason);

        onRefresh.RemoveDynamic(this, &AVoxelBody::RefreshDeformation);
        onDebugToggle.RemoveDynamic(this, &AVoxelBody::ToggleNodeDebug);
        onRotateToggle.RemoveDynamic(this, &AVoxelBody::TogglePlanetRotate);

        onDensityDelta.RemoveDynamic(this, &AVoxelBody::SetBrushDensity);
        onRadiusDelta.RemoveDynamic(this, &AVoxelBody::SetBrushRadius);
        onTypeDelta.RemoveDynamic(this, &AVoxelBody::SetBrushType);
        onLODToggle.RemoveDynamic(this, &AVoxelBody::ToggleLOD);
        onDeformToggle.RemoveDynamic(this, &AVoxelBody::ToggleDeform);
    }

protected:
    UVoxelMeshComponent* meshComponent;
};