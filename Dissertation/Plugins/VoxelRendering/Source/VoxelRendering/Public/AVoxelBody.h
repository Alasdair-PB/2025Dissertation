#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Octree.h"
#include "AVoxelBody.generated.h"

class UVoxelMeshComponent;
class UBufferResource;

UCLASS()
class VOXELRENDERING_API AVoxelBody : public AActor
{
    GENERATED_BODY()

public:
    AVoxelBody();
    static AVoxelBody* CreateVoxelMeshActor(UWorld* World, float scale, int size, int depth, int voxelsPerAxis, 
        TArray<float>& inIsovalueBuffer, TArray<uint32>& inTypeValueBuffer, 
        AActor* eraser, AActor* player);
};