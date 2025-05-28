#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"

struct FVoxelVertexInfo
{
	FVoxelVertexInfo() {}
	FVoxelVertexInfo(const FVector& InPosition, const FVector& InNormal, const FColor& InColor) :
		Position(InPosition)
	{
	}

	FVector Position;
	FVector Normal;
};
