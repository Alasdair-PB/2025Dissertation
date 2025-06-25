#include "VoxelWorldSubsystem.h"
#include "VoxelSceneViewExtension.h"


void UVoxelWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    SceneViewExtension = FSceneViewExtensions::NewExtension<FVoxelSceneViewExtension>(GetWorld());
}

void UVoxelWorldSubsystem::Deinitialize()
{
    SceneViewExtension.Reset();
    Super::Deinitialize();
}
