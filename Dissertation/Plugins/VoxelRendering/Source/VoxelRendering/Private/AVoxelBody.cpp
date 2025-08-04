#include "AVoxelBody.h"
#include "VoxelMeshComponent.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"

FOnGlobalEvent AVoxelBody::OnGlobalEvent;

AVoxelBody::AVoxelBody()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

void AVoxelBody::SetMeshComponent(UVoxelMeshComponent* inMeshComponent) {
    meshComponent = inMeshComponent;
}

AVoxelBody* AVoxelBody::CreateVoxelMeshActor(UWorld* World, float scale, int size, int depth, int voxelsPerAxis,
    TArray<float>& inIsovalueBuffer, TArray<uint32>& inTypeValueBuffer, AActor* eraser, AActor* player)
{
    if (!World) return nullptr;

    FActorSpawnParameters SpawnParams;

    UClass* voxelBodyClass = StaticLoadClass(AVoxelBody::StaticClass(), nullptr, TEXT("/VoxelRendering/BP_VoxelBody.BP_VoxelBody_C"));
    if (!voxelBodyClass) return nullptr;

    AVoxelBody* newActor = World->SpawnActor<AVoxelBody>(voxelBodyClass, SpawnParams);
    if (!newActor) return nullptr;

    UVoxelMeshComponent* voxelMesh = NewObject<UVoxelMeshComponent>(newActor);
    if (!voxelMesh) return nullptr;

    voxelMesh->AttachToComponent(newActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    voxelMesh->RegisterComponent();
    voxelMesh->InitVoxelMesh(scale, size, depth, voxelsPerAxis, inIsovalueBuffer, inTypeValueBuffer, eraser, player);

    newActor->SetMeshComponent(voxelMesh);
    return newActor;
}

void AVoxelBody::BroadcastGlobalEvent()
{
    OnGlobalEvent.Broadcast();
}

/*AVoxelBody* AVoxelBody::CreateVoxelMeshActor(UWorld* World, float scale, int size, int depth, int voxelsPerAxis,
    TArray<float>& inIsovalueBuffer, TArray<uint32>& inTypeValueBuffer, AActor* eraser, AActor* player)
{
    if (!World) return nullptr;

    FActorSpawnParameters SpawnParams;
    AVoxelBody* newActor = World->SpawnActor<AVoxelBody>(AVoxelBody::StaticClass(), SpawnParams);
    if (!newActor) return nullptr;

    UVoxelMeshComponent* voxelMesh = NewObject<UVoxelMeshComponent>(newActor);
    if (!voxelMesh) return nullptr;

    voxelMesh->AttachToComponent(newActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    voxelMesh->RegisterComponent();
    voxelMesh->InitVoxelMesh(scale, size, depth, voxelsPerAxis, inIsovalueBuffer, inTypeValueBuffer, eraser, player);

    newActor->SetMeshComponent(voxelMesh);
    return newActor;
}*/

void AVoxelBody::ToggleNodeDebug() {
    if (!meshComponent) return;
    meshComponent->ToggleDebugNode();
}

void AVoxelBody::TogglePlanetRotate() {
    if (!meshComponent) return;
    meshComponent->TogglePlanetRotate();
}

void AVoxelBody::RefreshDeformation() {
    if (!meshComponent) return;
    meshComponent->RefreshDeformation();
}

void AVoxelBody::SetBrushDensity(float density) {
    if (!meshComponent) return;
    meshComponent->SetBrushDensity(density);
}

void AVoxelBody::SetBrushRadius(float radius) {
    if (!meshComponent) return;
    meshComponent->SetBrushRadius(radius);
}

void AVoxelBody::SetBrushType(int type) {
    if (!meshComponent) return;
    meshComponent->SetPaintType(type);
}