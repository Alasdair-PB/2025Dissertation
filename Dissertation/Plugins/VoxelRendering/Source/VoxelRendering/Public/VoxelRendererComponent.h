#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "VoxelRendererComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class  VOXELRENDERING_API UVoxelRendererComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UVoxelRendererComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void RenderVoxelAsMarchingCubes(const TArray<float>& ScalarField, const FIntVector& GridSize, float IsoLevel);

private:
	UProceduralMeshComponent* ProcMesh;
};
