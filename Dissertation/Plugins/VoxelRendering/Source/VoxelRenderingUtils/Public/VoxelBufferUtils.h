#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"

struct VOXELRENDERINGUTILS_API FVoxelVertexInfo
{
	FVector Position; 
	FVector Normal;

	FVoxelVertexInfo() = default;

	FVoxelVertexInfo(const FVector& InPosition, const FVector& InNormal)
		: Position(InPosition), Normal(InNormal) {
	}
};
