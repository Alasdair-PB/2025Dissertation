#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../../Octree/Public/Octree.h"
#include "../../Octree/Public/OctreeNode.h"
#include "ProceduralMeshComponent.h"
#include "../../ComputeDispatchers/Public/MarchingCubesDispatcher.h"
#include "../../ComputeDispatchers/Public/PlanetGeneratorDispatcher.h"
#include "StopWatch.h"
#include "VoxelMeshComponent.h"
#include "VoxelGeneratorComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VOXELGENERATION_API UVoxelGeneratorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UVoxelGeneratorComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
private:
	void TraverseAndDraw(OctreeNode* node);
	void InitIsoDispatch();
	UProceduralMeshComponent* ProcMesh;

	void DispatchIsoBuffer(int size, int depth, float scale, int voxelsPerAxis);
	void InitVoxelMesh(int size, int depth, float scale, int voxelsPerAxis);
	UVoxelMeshComponent* voxelMesh;

	AABB bounds;
	TArray<float> isoValueBuffer;
	TArray<uint32> typeValueBuffer;
	StopWatch* stopWatch = new StopWatch();
};
