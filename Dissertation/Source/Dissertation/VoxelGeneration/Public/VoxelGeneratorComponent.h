#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../../Octrees/Public/Octree.h"
#include "../../VoxelRenderer/Public/VoxelRendererComponent.h"
#include "VoxelGeneratorComponent.generated.h"
#include "MarchingCubesDispatcher.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DISSERTATION_API UVoxelGeneratorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UVoxelGeneratorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	void TraverseAndDraw(OctreeNode* node);
	void InitOctree();
	void InvokeVoxelRenderer(OctreeNode* node);
	void DispatchMarchingCubes(OctreeNode* node, uint32 depth);
	void SampleExampleComputeShader();
	void SwapBuffers();
	void UpdateMesh(FMarchingCubesOutput meshInfo);

	UVoxelRendererComponent* voxelRenderer;
	Octree* tree;
	uint32 nodeIndex = 0;

	FMarchingCubesOutput marchingCubesOutBuffer[2];
	bool bBufferReady[2] = { false, false };
	int32 ReadBufferIndex = 0;
	int32 WriteBufferIndex = 1;


	int32 ReadBufferIndex = 0;
	int32 WriteBufferIndex = 1;
	
	float SampleSDF(FVector p);

};
