#include "VoxelMeshComponent.h"
#include "FVoxelVertexFactory.h"
#include "FVoxelSceneProxy.h"

#include "FVoxelVertexFactoryShaderParameters.h"


FPrimitiveSceneProxy* UVoxelMeshComponent::CreateSceneProxy()
{
	return new FVoxelSceneProxy(this, GetWorld()->GetFeatureLevel());
}


UVoxelMeshComponent::UVoxelMeshComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bUseAsOccluder = false;

    static ConstructorHelpers::FObjectFinder<UMaterial> MaterialAsset(TEXT("/Game/Materials/M_VoxelPixelShader"));
    if (MaterialAsset.Succeeded())
    {
        MaterialInstance = UMaterialInstanceDynamic::Create(MaterialAsset.Object, this);
    }
}

FPrimitiveSceneProxy* UVoxelMeshComponent::CreateSceneProxy()
{
    if (!MaterialInstance)
        return nullptr;
    return new FVoxelSceneProxy(this, MaterialInstance);
}

FBoxSphereBounds UVoxelMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
    return FBoxSphereBounds(FBox(FVector(-100), FVector(100))); // Replace with actual bounds
}

void UVoxelMeshComponent::UpdateMesh(const TArray<FVoxelVertexInfo>& Vertices, const TArray<uint32>& Indices)
{

    MarkRenderStateDirty();
}