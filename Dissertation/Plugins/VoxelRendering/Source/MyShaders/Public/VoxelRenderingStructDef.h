#pragma once
#include "CoreMinimal.h"

USTRUCT(BlueprintType)
struct FOctreeNode
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "OctreeNode")
    FVector3f voxelBodyCoord;

    UPROPERTY(BlueprintReadWrite, Category = "OctreeNode")
    int32 scalarFieldOffset;

    UPROPERTY(BlueprintReadWrite, Category = "OctreeNode")
    int32 depthLevel;
};

USTRUCT(BlueprintType)
struct FVertex
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Vertex")
    FVector3f position;

    UPROPERTY(BlueprintReadWrite, Category = "Vertex")
    FVector3f normal;

    UPROPERTY(BlueprintReadWrite, Category = "Vertex")
    FVector2f id;
};

USTRUCT(BlueprintType)
struct FTriangle
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Triangle")
    FVertex vertexC;

    UPROPERTY(BlueprintReadWrite, Category = "Triangle")
    FVertex vertexB;

    UPROPERTY(BlueprintReadWrite, Category = "Triangle")
    FVertex vertexA;
};
