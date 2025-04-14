#include "VoxelGeneratorComponent.h"

UVoxelGeneratorComponent::UVoxelGeneratorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UVoxelGeneratorComponent::BeginPlay()
{
	Super::BeginPlay();
    AActor* Owner = GetOwner();
    voxelRenderer = NewObject<UVoxelRendererComponent>(Owner);
    voxelRenderer->RegisterComponent();
    InitOctree();
}

void UVoxelGeneratorComponent::InitOctree() {
    AABB bounds = { FVector(-200.0f), FVector(200.0f) };
    tree = new Octree(bounds, 5);
    tree->Build([this](FVector p) { return SampleSDF(p); });
}

float UVoxelGeneratorComponent::SampleSDF(FVector p) {
    return (p.Length()) - 1.0f;
}

void UVoxelGeneratorComponent::TraverseAndDraw(OctreeNode* node) {
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
void UVoxelGeneratorComponent::InvokeVoxelRenderer(OctreeNode* node) {
    if (!node) return;

    FVector min = node->bounds.min;
    FVector max = node->bounds.max;
    FVector nodeSize = max - min;

    int resolution = FMath::RoundToInt(FMath::Max(nodeSize.X, FMath::Max(nodeSize.Y, nodeSize.Z)) / 10.0f);
    FIntVector GridSize(resolution, resolution, resolution);
    TArray<float> ScalarField;
    ScalarField.SetNumUninitialized(GridSize.X * GridSize.Y * GridSize.Z);

    int index = 0;
    for (int x = 0; x < GridSize.X; ++x) {
        for (int y = 0; y < GridSize.Y; ++y) {
            for (int z = 0; z < GridSize.Z; ++z) {
                FVector p = FMath::Lerp(min, max, FVector(
                    (float)x / (GridSize.X - 1),
                    (float)y / (GridSize.Y - 1),
                    (float)z / (GridSize.Z - 1)
                ));

                float value = SampleSDF(p);
                ScalarField[index++] = value;
            }
        }
    }

    float IsoLevel = 2.0f;
    voxelRenderer->RenderVoxelAsMarchingCubes(ScalarField, GridSize, IsoLevel);
}

void UVoxelGeneratorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    TraverseAndDraw((*tree).root);
    InvokeVoxelRenderer((*tree).root);
}

