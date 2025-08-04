#include "AVoxelBody.h"
#include "VoxelMeshComponent.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"

FOnRefresh AVoxelBody::onRefresh;
FOnDebugToggle AVoxelBody::onDebugToggle;
FOnRotateToggle AVoxelBody::onRotateToggle;
FOnLODToggle AVoxelBody::onLODToggle;
FOnDeformToggle AVoxelBody::onDeformToggle;

FOnDensityDelta AVoxelBody::onDensityDelta;
FOnRadiusDelta AVoxelBody::onRadiusDelta;
FOnTypeDelta AVoxelBody::onTypeDelta;


AVoxelBody::AVoxelBody()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

void AVoxelBody::SetMeshComponent(UVoxelMeshComponent* inMeshComponent) {
    meshComponent = inMeshComponent;
}

void AVoxelBody::BroadcastDensityDeltaEvent(float density) {
    onDensityDelta.Broadcast(density);
}

void AVoxelBody::BroadcastRadiusDeltaEvent(float radius) {
    onRadiusDelta.Broadcast(radius);
}

void AVoxelBody::BroadcastTypeDeltaEvent(int type) {
    onTypeDelta.Broadcast(type);
}

void AVoxelBody::BroadcastRotateToggleEvent() {
    onRotateToggle.Broadcast();
}

void AVoxelBody::BroadcastDebugToggleEvent() {
    onDebugToggle.Broadcast();
}

void AVoxelBody::BroadcastRefreshEvent() {
    onRefresh.Broadcast();
}

void AVoxelBody::BroadcastLODToggleEvent() {
    onLODToggle.Broadcast();
}

void AVoxelBody::BroadcastDeformToggleEvent() {
    onDeformToggle.Broadcast();
}

AVoxelBody* AVoxelBody::CreateVoxelMeshActor(UWorld* World, float scale, int size, int depth, int voxelsPerAxis,
    TArray<float>& inIsovalueBuffer, TArray<uint32>& inTypeValueBuffer, AActor* eraser, AActor* player, UNiagaraSystem* vfxSystem)
{
    if (!World) return nullptr;

    FActorSpawnParameters SpawnParams;
    AVoxelBody* newActor = World->SpawnActor<AVoxelBody>(AVoxelBody::StaticClass(), SpawnParams);
    if (!newActor) return nullptr;

    UVoxelMeshComponent* voxelMesh = NewObject<UVoxelMeshComponent>(newActor);
    if (!voxelMesh) return nullptr;

    voxelMesh->AttachToComponent(newActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    voxelMesh->RegisterComponent();
    voxelMesh->InitVoxelMesh(scale, size, depth, voxelsPerAxis, inIsovalueBuffer, inTypeValueBuffer, eraser, player, vfxSystem);

    newActor->SetMeshComponent(voxelMesh);
    return newActor;
}

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

void AVoxelBody::ToggleLOD() {
    if (!meshComponent) return;
    meshComponent->ToggleLODState();
}


void AVoxelBody::ToggleDeform() {
    if (!meshComponent) return;
    meshComponent->ToggleDeform();
}

