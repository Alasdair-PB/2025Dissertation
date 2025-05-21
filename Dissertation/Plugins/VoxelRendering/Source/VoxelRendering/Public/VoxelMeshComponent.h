#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "FVoxelVertexFactory.h"
#include "VoxelMeshComponent.generated.h"

class FPrimitiveSceneProxy;

UCLASS()
class VOXELRENDERING_API UVoxelMeshComponent : public UMeshComponent
{
    GENERATED_BODY()

public:
    UVoxelMeshComponent();

    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

    void UpdateMesh(const TArray<FVoxelVertexInfo>& Vertices, const TArray<uint32>& Indices);

private:
    UMaterialInstanceDynamic* MaterialInstance;
};
