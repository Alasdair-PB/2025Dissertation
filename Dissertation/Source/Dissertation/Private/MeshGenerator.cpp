// Fill out your copyright notice in the Description page of Project Settings.
#include "MeshGenerator.h"

UMeshGenerator::UMeshGenerator()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMeshGenerator::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	ProcMesh = NewObject<UProceduralMeshComponent>(Owner);
	ProcMesh->RegisterComponent();
	ProcMesh->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	GenerateMesh();
}

void UMeshGenerator::GenerateCube() {
    FVector Vertices[8] = {
       FVector(0, 0, 0),
       FVector(0, 100, 0),
       FVector(100, 100, 0),
       FVector(100, 0, 0),
       FVector(0, 0, 100),
       FVector(0, 100, 100),
       FVector(100, 100, 100),
       FVector(100, 0, 100)
    };

    TArray<FVector> VertArray = {
        Vertices[0], Vertices[1], Vertices[2], Vertices[3],
        Vertices[4], Vertices[5], Vertices[6], Vertices[7]
    };

    TArray<int32> Triangles = {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7
    };

    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;
    TArray<FLinearColor> VertexColors; // FColor for CreateMeshSection, FLinear otherwise

    for (int i = 0; i < VertArray.Num(); i++) {
        Normals.Add(FVector(0, 0, 1));
        UVs.Add(FVector2D(0, 0));
        Tangents.Add(FProcMeshTangent(1, 0, 0));
        VertexColors.Add(FColor::White);
    }
    //ProcMesh->CreateMeshSection(0, VertArray, Triangles, Normals, UVs, VertexColors, Tangents, true);
    ProcMesh->CreateMeshSection_LinearColor(0, VertArray, Triangles, Normals, UVs, VertexColors, Tangents, true, false);
}

void UMeshGenerator::GenerateMesh() {
    //GenerateCube();
}

void UMeshGenerator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

