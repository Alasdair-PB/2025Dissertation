#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../../Octrees/Public/Octree.h"
#include "../../VoxelRenderer/Public/VoxelRendererComponent.h"
#include "VoxelGeneratorComponent.generated.h"

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

	UVoxelRendererComponent* voxelRenderer;
	Octree* tree;
	float SampleSDF(FVector p);
};
