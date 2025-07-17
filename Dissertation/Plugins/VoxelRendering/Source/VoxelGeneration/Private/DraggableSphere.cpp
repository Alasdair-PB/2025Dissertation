#include "DraggableSphere.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UDraggableSphere::UDraggableSphere()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UDraggableSphere::BeginPlay()
{
    Super::BeginPlay();

    PlayerController = GetWorld()->GetFirstPlayerController();
    if (PlayerController)
    {
        PlayerController->bShowMouseCursor = true;
        PlayerController->bEnableClickEvents = true;
        PlayerController->bEnableMouseOverEvents = true;
        PlayerController->DefaultMouseCursor = EMouseCursor::Default;
        PlayerController->SetInputMode(FInputModeGameAndUI());
    }

    Sphere = Cast<UStaticMeshComponent>(targetUMesh->GetComponentByClass(UStaticMeshComponent::StaticClass()));

    if (!Sphere)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find StaticMeshComponent on actor %s"), *targetUMesh->GetName());
        return;
    }

    Sphere->SetSimulatePhysics(false);
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Sphere->SetCollisionObjectType(ECC_WorldDynamic);
    Sphere->SetCollisionResponseToAllChannels(ECR_Block);
    Sphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Sphere->SetGenerateOverlapEvents(true);
    Sphere->SetMobility(EComponentMobility::Movable);

    Sphere->OnClicked.AddUniqueDynamic(this, &UDraggableSphere::OnClicked);
    Sphere->OnReleased.AddUniqueDynamic(this, &UDraggableSphere::OnReleased);
}


void UDraggableSphere::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsDragging)
    {
        if (!PlayerController->IsInputKeyDown(EKeys::LeftMouseButton)) StopDrag();
        else UpdateDrag();
    }
}

void UDraggableSphere::OnClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
    if (ButtonPressed == EKeys::LeftMouseButton)
        StartDrag();
}

void UDraggableSphere::OnReleased(UPrimitiveComponent* TouchedComponent, FKey ButtonReleased)
{
    if (ButtonReleased == EKeys::LeftMouseButton)
        StopDrag();
}


void UDraggableSphere::StartDrag()
{
    bIsDragging = true;
    FVector WorldLoc, WorldDir;
    if (PlayerController->DeprojectMousePositionToWorld(WorldLoc, WorldDir))
    {
        FHitResult Hit;
        FVector Start = WorldLoc;
        FVector End = Start + WorldDir * 10000;

        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
        {
            FVector CameraLocation;
            FRotator CameraRotation;
            PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
            FVector PlaneNormal = -CameraRotation.Vector();
            FPlane DragPlane(targetUMesh->GetActorLocation(), PlaneNormal);

            FVector IntersectionPoint;
            if (FMath::SegmentPlaneIntersection(Hit.TraceStart, Hit.TraceEnd, DragPlane, IntersectionPoint))
                DragOffset = targetUMesh->GetActorLocation() - IntersectionPoint;
        }
    }
}

void UDraggableSphere::StopDrag()
{
    bIsDragging = false;
}

void UDraggableSphere::UpdateDrag()
{
    FVector2D MousePos;
    if (PlayerController && PlayerController->GetMousePosition(MousePos.X, MousePos.Y))
    {
        FVector WorldOrigin, WorldDirection;
        if (PlayerController->DeprojectScreenPositionToWorld(MousePos.X, MousePos.Y, WorldOrigin, WorldDirection))
        {
            FVector ActorLocation = GetOwner()->GetActorLocation();

            FVector CameraLocation;
            FRotator CameraRotation;
            PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
            FVector PlaneNormal = -CameraRotation.Vector();
            FVector CameraForward = PlayerController->PlayerCameraManager->GetActorForwardVector();
            FPlane DragPlane(ActorLocation, CameraForward);

            FVector IntersectionPoint;
            bool bHit = FMath::SegmentPlaneIntersection(
                WorldOrigin,
                WorldOrigin + WorldDirection * 10000.f,
                DragPlane,
                IntersectionPoint
            );

            if (bHit)
            {
                FVector NewLocation = IntersectionPoint + DragOffset;
                targetUMesh->SetActorLocation(NewLocation);
            }
        }
    }
}
