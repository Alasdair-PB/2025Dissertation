#include "VoxelWorldSubsystem.h"
#include "VoxelSceneViewExtension.h"
#include "SceneViewExtensions.h"

void UVoxelWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    SceneViewExtension = FSceneViewExtensions::NewWorldExtension<FVoxelSceneViewExtension>(*GetWorld());
}

void UVoxelWorldSubsystem::Deinitialize()
{
    SceneViewExtension.Reset();
    Super::Deinitialize();
}
