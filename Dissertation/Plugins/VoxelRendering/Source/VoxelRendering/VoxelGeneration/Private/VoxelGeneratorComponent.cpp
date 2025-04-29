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
    leafCount = GetLeafCount((*tree).root);
}

void UVoxelGeneratorComponent::InitOctree() {
    AABB bounds = { FVector(-200.0f), FVector(200.0f) };
    tree = new Octree(bounds, 5);
    tree->Build([this](FVector p) { return SampleSDF(p); });
}

float UVoxelGeneratorComponent::SampleSDF(FVector p) {
    return (p.Length()) - 1.0f;
}

void UVoxelGeneratorComponent::SampleExampleComputeShader() {
    FMySimpleComputeShaderDispatchParams Params(1, 1, 1);
    Params.Input[0] = 2;
    Params.Input[1] = 5;

    FMySimpleComputeShaderInterface::Dispatch(Params, [](int OutputVal) {
        UE_LOG(LogTemp, Warning, TEXT("This is a debug message with value: %d"), OutputVal);
        });
}

void UVoxelGeneratorComponent::SwapBuffers() {
    int32 Temp = ReadBufferIndex;
    ReadBufferIndex = WriteBufferIndex;
    WriteBufferIndex = Temp;
}

int UVoxelGeneratorComponent::GetLeafCount(OctreeNode* node) {
    if (!node) return 0;
    if (node->isLeaf) return 1;
    int count = 0;
    for (int i = 0; i < 8; ++i)
        count += GetLeafCount(node->children[i]);
    return count;
}

void UVoxelGeneratorComponent::UpdateMesh(FMarchingCubesOutput meshInfo) {
    //UE_LOG(LogTemp, Warning, TEXT("This is a debug message with value: %d"), meshInfo.tris[0]);
    for (int32 tri : meshInfo.tris) {
        UE_LOG(LogTemp, Warning, TEXT("A tri has been returned"));
    }
    for (FVector3f vert : meshInfo.vertices) {
        UE_LOG(LogTemp, Warning, TEXT("A vertex has been returned"));
    }
    for (FVector3f normal : meshInfo.normals) {
        UE_LOG(LogTemp, Warning, TEXT("A normal value has been returned"));
    }
}

void UVoxelGeneratorComponent::InvokeVoxelRenderer(OctreeNode* node) {
    // SampleExampleComputeShader();
    if (bBufferReady[ReadBufferIndex])
    {
        UpdateMesh(marchingCubesOutBuffer[ReadBufferIndex]);
        bBufferReady[ReadBufferIndex] = false;
        SwapBuffers();
    }

    FMarchingCubesDispatchParams Params(1, 1, 1);
    Params.Input.baseDepthScale = 0.5f;
    Params.Input.isoLevel = 2;
    Params.Input.voxelsPerNode = voxelBodyDimensions;
    Params.Input.tree = node;
    Params.Input.leafCount = leafCount;

    FMarchingCubesInterface::Dispatch(Params, [this](FMarchingCubesOutput OutputVal) {
        bBufferReady[ReadBufferIndex] = true;
        //UE_LOG(LogTemp, Warning, TEXT("Dispatch returned"));
     });
}

void UVoxelGeneratorComponent::TraverseAndDraw(OctreeNode* node) {
    if (!node) return;

    if (node->isLeaf) {
        DrawDebugBox(GetWorld(), node->bounds.Center(), node->bounds.Extent(), FColor::Green, false, -1.f, 0, 1.f);
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

