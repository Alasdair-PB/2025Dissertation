#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../../../Octree/Public/Octree.h"
#include "../../../Octree/Public/OctreeNode.h"
#include "ProceduralMeshComponent.h"
#include "VoxelRendererComponent.h"
#include "../../../MyShaders/Public/MarchingCubesDispatcher.h"
#include "../../../MyShaders/Public/PlanetGeneratorDispatcher.h"
#include "StopWatch.h"
#include "VoxelGeneratorComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VOXELRENDERING_API UVoxelGeneratorComponent : public UActorComponent
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
	void InitOctree();
	void InvokeVoxelRenderer(OctreeNode* node);
	int GetLeafCount(OctreeNode* node);
	UProceduralMeshComponent* ProcMesh;

	void SampleExampleComputeShader();
	void SwapBuffers();
	void UpdateMesh(FMarchingCubesOutput meshInfo);
	void DispatchIsoBuffer(int size, int depth);
	void BuildOctree(int size, int depth);
	UVoxelRendererComponent* voxelRenderer;
	Octree* tree;
	float SampleSDF(FVector3f p);
	FMarchingCubesOutput marchingCubesOutBuffer[2];
	bool bBufferReady[2] = { false, false };

	TArray<float> isovalueBuffer;
	uint32 ReadBufferIndex = 0;
	uint32 WriteBufferIndex = 1;
	uint32 nodeIndex = 0;
	StopWatch* stopWatch = new StopWatch();

	int leafCount = 0;
};
