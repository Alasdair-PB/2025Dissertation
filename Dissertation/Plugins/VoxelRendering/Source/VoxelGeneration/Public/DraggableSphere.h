#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DraggableSphere.generated.h"

class USphereComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VOXELGENERATION_API UDraggableSphere : public UActorComponent
{
    GENERATED_BODY()

public:
    UDraggableSphere();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
    AActor* targetUMesh;
    UStaticMeshComponent* Sphere;

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool bIsDragging;
    FVector DragOffset;
    APlayerController* PlayerController;

    UFUNCTION()
    void OnClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

    UFUNCTION()
    void OnReleased(UPrimitiveComponent* TouchedComponent, FKey ButtonReleased);

    void StartDrag();
    void StopDrag();
    void UpdateDrag();
};
