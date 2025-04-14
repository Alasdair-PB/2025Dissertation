#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Octrees/Public/Octree.h"
#include "ProceduralMeshComponent.h"
#include "MeshGenerator.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DISSERTATION_API UMeshGenerator : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMeshGenerator();

protected:
	virtual void BeginPlay() override;

public:	
	void GenerateMesh();
	void GenerateCube();
private:
	UProceduralMeshComponent* ProcMesh;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	float SampleSDF(FVector p);
	Octree* tree;
	void TraverseAndDraw(OctreeNode* node);
	void InitOctree();

};
