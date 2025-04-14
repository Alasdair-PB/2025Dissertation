#include "VoxelRendererComponent.h"
#include "PolygoniseMappings.h"

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

    const FVector CubeCorners[8] = {
        FVector(0,0,0), FVector(1,0,0), FVector(1,1,0), FVector(0,1,0),
        FVector(0,0,1), FVector(1,0,1), FVector(1,1,1), FVector(0,1,1)
    };

    for (int x = 0; x < GridSize.X - 1; x++) {
        for (int y = 0; y < GridSize.Y - 1; y++) {
            for (int z = 0; z < GridSize.Z - 1; z++) {
                FVector CubePos(x, y, z);
                float CubeValues[8];
                FVector VertList[12];

                for (int i = 0; i < 8; i++) {
                    FIntVector Corner = FIntVector(CubePos + CubeCorners[i]);
                    int Index = Corner.X + GridSize.X * (Corner.Y + GridSize.Y * Corner.Z);
                    CubeValues[i] = ScalarField[Index];
                }

                int CubeIndex = 0;
                for (int i = 0; i < 8; i++) {
                    if (CubeValues[i] < IsoLevel)
                        CubeIndex |= (1 << i);
                }

                int EdgeFlags = EdgeTable[CubeIndex];
                if (EdgeFlags == 0) continue;

                for (int i = 0; i < 12; i++) {
                    if (EdgeFlags & (1 << i)) {
                        int v1 = EdgeVertexMap[i][0];
                        int v2 = EdgeVertexMap[i][1];

                        FVector p1 = CubePos + CubeCorners[v1];
                        FVector p2 = CubePos + CubeCorners[v2];
                        float valp1 = CubeValues[v1];
                        float valp2 = CubeValues[v2];

                        VertList[i] = FMath::Lerp(p1, p2, (IsoLevel - valp1) / (valp2 - valp1));
                    }
                }

                for (int i = 0; TriTable[CubeIndex][i] != -1; i += 3) {
                    int idx0 = Vertices.Add(VertList[TriTable[CubeIndex][i]]);
                    int idx1 = Vertices.Add(VertList[TriTable[CubeIndex][i + 1]]);
                    int idx2 = Vertices.Add(VertList[TriTable[CubeIndex][i + 2]]);

                    Indices.Add(idx0);
                    Indices.Add(idx1);
                    Indices.Add(idx2);
                }
            }
        }
    }

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

