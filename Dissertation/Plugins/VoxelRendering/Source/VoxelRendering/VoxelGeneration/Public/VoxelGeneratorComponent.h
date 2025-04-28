#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../../../Octree/Public/Octree.h"
#include "../../../Octree/Public/OctreeNode.h"

#include "VoxelRendererComponent.h"
#include "../../../MyShaders/Public/MarchingCubesDispatcher.h"
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

	FMarchingCubesOutput marchingCubesOutBuffer[2];
	bool bBufferReady[2] = { false, false };
	uint32 ReadBufferIndex = 0;
	uint32 WriteBufferIndex = 1;
	uint32 nodeIndex = 0;

	float SampleSDF(FVector p);

};
