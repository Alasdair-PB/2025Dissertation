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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	AActor* targetEraser;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	AActor* targetPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	int depth = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	int voxelsPerAxis = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	float scale = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	float planetScaleRatio = 0.8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	float fbmAmplitude = 0.2; // Reduce to make spikes less common

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	float fbmFrequency = 0.02; // Reduce to make neightbour isovalues more consistent

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	float voronoiScale = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	float voronoiJitter = 0.2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	float voronoiWeight = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	float fbmWeight = 0.3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	float surfaceWeight = 0.3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	float voronoiThreshold = 0.3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCategory")
	int surfaceLayers = 3;

	UVoxelGeneratorComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
private:
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
