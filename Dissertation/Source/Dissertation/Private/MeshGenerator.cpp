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

float UMeshGenerator::SampleSDF(FVector p) {
    return (p.Length()) - 1.0f;
}

void UMeshGenerator::TraverseAndDraw(OctreeNode* node) {
    if (!node) return;

    if (node->isLeaf) {
        DrawDebugBox(
            GetWorld(),
            node->bounds.Center(),
            node->bounds.Extent(),
            FColor::Green,
            false,
            -1.f,
            0,
            1.f
        );

        const int resolution = 2;
        FVector min = node->bounds.min;
        FVector max = node->bounds.max;

        for (int x = 0; x <= resolution; ++x) {
            for (int y = 0; y <= resolution; ++y) {
                for (int z = 0; z <= resolution; ++z) {
                    FVector p = FMath::Lerp(min, max, FVector(
                        (float)x / resolution,
                        (float)y / resolution,
                        (float)z / resolution
                    ));

                    float v = SampleSDF(p);
                    FColor color = (FMath::Abs(v) < 0.01f) ? FColor::White : (v < 0.f ? FColor::Blue : FColor::Red);

                    DrawDebugPoint(GetWorld(), p, 5.f, color, false, -1.f);
                }
            }
        }

        return;
    }

    for (int i = 0; i < 8; ++i)
        TraverseAndDraw(node->children[i]);
}

void UMeshGenerator::InitOctree() {
    AABB bounds = { FVector(-200.0f), FVector(200.0f) };
    tree = new Octree(bounds, 5);
    tree->Build([this](FVector p) { return SampleSDF(p); });
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
    InitOctree();
}

void UMeshGenerator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    TraverseAndDraw((*tree).root);
}

