#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Octree.h"
#include "Delegates/DelegateCombinations.h"
#include "AVoxelBody.generated.h"

class UVoxelMeshComponent;
class UBufferResource;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGlobalEvent);

UCLASS()
class VOXELRENDERING_API AVoxelBody : public AActor
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Events")
    static void BroadcastGlobalEvent();

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnRefreshButtonClicked();

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnTogglePlanetRotationClicked();

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnToggleDebugNodesClicked();

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnSliderBrushDensityModified(float density);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnSliderBrushRadiusModified(float radius);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnDropDownBrushTypeModified(int type);

    AVoxelBody();
    static AVoxelBody* CreateVoxelMeshActor(UWorld* World, float scale, int size, int depth, int voxelsPerAxis, 
        TArray<float>& inIsovalueBuffer, TArray<uint32>& inTypeValueBuffer, 
        AActor* eraser, AActor* player);

    void SetMeshComponent(UVoxelMeshComponent* inMeshComponent);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleNodeDebug();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void RefreshDeformation();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void TogglePlanetRotate();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetBrushDensity(float density);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetBrushRadius(float radius);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetBrushType(int type);

    static FOnGlobalEvent OnGlobalEvent;

    void BeginPlay()
    {
        Super::BeginPlay();
        OnGlobalEvent.AddDynamic(this, &AVoxelBody::ToggleNodeDebug);
    }

protected:
    UVoxelMeshComponent* meshComponent;
};