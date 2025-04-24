#include "VoxelGeneratorComponent.h"
#include "MySimpleComputeShader.h"
#include "Logging/LogMacros.h"

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

void UVoxelGeneratorComponent::InvokeVoxelRenderer(OctreeNode* node) {
    FMySimpleComputeShaderDispatchParams Params(1, 1, 1);
    Params.Input[0] = 2;
    Params.Input[1] = 5;

    FMySimpleComputeShaderInterface::Dispatch(Params, [](int OutputVal) {
        UE_LOG(LogTemp, Warning, TEXT("This is a debug message with value: %d"), OutputVal);
    });
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

void UVoxelGeneratorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    TraverseAndDraw((*tree).root);
    InvokeVoxelRenderer((*tree).root);
}

