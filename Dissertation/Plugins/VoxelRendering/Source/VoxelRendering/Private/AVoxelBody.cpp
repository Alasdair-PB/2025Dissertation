#include "AVoxelBody.h"
#include "VoxelMeshComponent.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"

AVoxelBody::AVoxelBody()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

AVoxelBody* AVoxelBody::CreateVoxelMeshActor(UWorld* World, float scale, int size, int depth, int voxelsPerAxis, 
    TArray<float>& inIsovalueBuffer, TArray<uint32>& inTypeValueBuffer, AActor* eraser, AActor* player)
{
    if (!World) return nullptr;

    FActorSpawnParameters SpawnParams;
    AVoxelBody* NewActor = World->SpawnActor<AVoxelBody>(AVoxelBody::StaticClass(), SpawnParams);
    if (!NewActor) return nullptr;

    UVoxelMeshComponent* VoxelMesh = NewObject<UVoxelMeshComponent>(NewActor);
    if (!VoxelMesh) return nullptr;

    VoxelMesh->AttachToComponent(NewActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    VoxelMesh->RegisterComponent();
    VoxelMesh->InitVoxelMesh(scale, size, depth, voxelsPerAxis, inIsovalueBuffer, inTypeValueBuffer, eraser, player);
    return NewActor;
}
