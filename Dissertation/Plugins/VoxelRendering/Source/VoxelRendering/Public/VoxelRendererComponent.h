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
	UVoxelRendererComponent() {}
private:
	UProceduralMeshComponent* ProcMesh;
};
