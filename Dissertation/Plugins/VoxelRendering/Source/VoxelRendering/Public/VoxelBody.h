/*#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"

#include "Curves/CurveFloat.h"
#include "HAL/ThreadSafeBool.h"
#include "Engine/EngineTypes.h"
#include "Engine/LatentActionManager.h"
#include "ConvexVolume.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "GameFramework/Volume.h"
#include "VoxelCollisionRendering.h"

#include "Octree.h"
#include "OctreeNode.h"
#include "VoxelBody.generated.h"

class UVoxelMeshComponent;
class UBodySetup;

UCLASS(BlueprintType, AutoExpandCategories = ("Performance", "Rendering|Sprite"), AutoCollapseCategories = ("Import Settings"))
class UVoxelBody : public UObject, public IInterface_CollisionDataProvider
{
protected:
	Octree* tree;
	FVoxelCollisionRendering* CollisionRendering;

	GENERATED_BODY()
	UPROPERTY(transient, duplicatetransient)
	TObjectPtr<UBodySetup> BodySetup;
	UPROPERTY(transient, duplicatetransient)
	TObjectPtr<UBodySetup> NewBodySetup;
public:
	UBodySetup* GetBodySetup();

};*/