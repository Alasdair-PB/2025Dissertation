#include "VoxelRendererComponent.h"

UVoxelRendererComponent::UVoxelRendererComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UVoxelRendererComponent::BeginPlay()
{
	Super::BeginPlay();
    AActor* Owner = GetOwner();
    ProcMesh = NewObject<UProceduralMeshComponent>(Owner);
    ProcMesh->RegisterComponent();
    ProcMesh->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
}

void UVoxelRendererComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UVoxelRendererComponent::RenderVoxelAsMarchingCubes(const TArray<float>& ScalarField, const FIntVector& GridSize, float IsoLevel)
{
    TArray<FVector> Vertices;
    TArray<int32> Indices;

    // Set vertices here

    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;
    TArray<FLinearColor> VertexColors;

    for (int i = 0; i < Vertices.Num(); i++) {
        Normals.Add(FVector(0, 0, 1));
        UVs.Add(FVector2D(0, 0));
        Tangents.Add(FProcMeshTangent(1, 0, 0));
        VertexColors.Add(FColor::White);
    }
    ProcMesh->CreateMeshSection_LinearColor(0, Vertices, Indices, Normals, UVs, VertexColors, Tangents, true, false);
}

