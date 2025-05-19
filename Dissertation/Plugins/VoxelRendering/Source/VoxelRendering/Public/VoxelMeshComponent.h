#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"

// Placeholder headers
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "PhysicsEngine/ConvexElem.h"
#include "Engine/StaticMesh.h"
// end placeholder

#include "VoxelMeshComponent.generated.h"
class FPrimitiveSceneProxy;

UCLASS()
class VOXELRENDERING_API UVoxelMeshComponent : public UMeshComponent
{
	GENERATED_BODY()
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
};
