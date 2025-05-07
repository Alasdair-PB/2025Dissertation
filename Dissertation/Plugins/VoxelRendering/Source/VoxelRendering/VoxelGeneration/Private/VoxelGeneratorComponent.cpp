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
    ProcMesh = NewObject<UProceduralMeshComponent>(Owner);
    ProcMesh->RegisterComponent();
    ProcMesh->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
}

void UVoxelGeneratorComponent::BeginDestroy() {
    Super::BeginDestroy();
    //voxelRenderer->DestroyComponent();
    delete tree;
}

void UVoxelGeneratorComponent::InitOctree() {
    AABB bounds = { FVector3f(-200.0f), FVector3f(200.0f) };
    tree = new Octree(bounds, 2);

    TArray<float> isovalueBuffer;
    TArray<uint8> typeBuffer;

    int sx = 2, sy = 2, sz = 2;
    int numVoxels = sx * sy * sz;

    for (int z = 0; z < sz; ++z) {
        for (int y = 0; y < sy; ++y) {
            for (int x = 0; x < sx; ++x) {
                float iso = (x + y + z) % 2 == 0 ? -1.0f : 1.0f;
                uint8 type = (x + y + z) % 2;
                isovalueBuffer.Add(iso);
                typeBuffer.Add(type);
            }
        }
    }
    tree->BuildFromBuffers(isovalueBuffer, typeBuffer, sx, sy, sz);
}

float UVoxelGeneratorComponent::SampleSDF(FVector3f p) {
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
    TArray<FVector> Vertices;
    TArray<int32> Indices;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;
    TArray<FLinearColor> VertexColors;
    TMap<int32, int32> IndexRemap;

    int32 currentIndex = 0;

    for (int32 i = 0; i < meshInfo.outVertices.Num(); i++) {
        const FVector& V = FVector(meshInfo.outVertices[i]);
        const FVector& N = FVector(meshInfo.outNormals[i]);

        if (N == FVector(-1, -1, -1)) continue;
        IndexRemap.Add(i, currentIndex);
        Vertices.Add(V);
        Normals.Add(N);
        UVs.Add(FVector2D(0, 0));
        Tangents.Add(FProcMeshTangent(1, 0, 0));
        VertexColors.Add(FColor::White);
        currentIndex++;
    }

    for (int32 i = 0; i < meshInfo.outTris.Num(); i += 3) {
        int32 A = meshInfo.outTris[i];
        int32 B = meshInfo.outTris[i + 1];
        int32 C = meshInfo.outTris[i + 2];

        if (A == -1 || B == -1 || C == -1) continue;
        if (!IndexRemap.Contains(A) || !IndexRemap.Contains(B) || !IndexRemap.Contains(C)) { 
            UE_LOG(LogTemp, Warning, TEXT("Bad triangle: %d"), A);
            continue; 
        }

        int32 IA = IndexRemap[A];
        int32 IB = IndexRemap[B];
        int32 IC = IndexRemap[C];

        if (IA == IB || IB == IC || IC == IA) {
            UE_LOG(LogTemp, Warning, TEXT("Bad triangle: %d"), A);
            continue;
        }
        if (Vertices[IA] == Vertices[IB] || Vertices[IB] == Vertices[IC] || Vertices[IC] == Vertices[IA]) {
            UE_LOG(LogTemp, Warning, TEXT("Bad triangle: %d"), A);
            continue;
        }

        Indices.Add(IA);
        Indices.Add(IB);
        Indices.Add(IC);
    }

    /*for (int32 i = 0; i < meshInfo.outTris.Num(); i += 3) {
        int32 A = meshInfo.outTris[i];
        int32 B = meshInfo.outTris[i + 1];
        int32 C = meshInfo.outTris[i + 2];

        if (A == -1 || B == -1 || C == -1) continue; 
        if (!IndexRemap.Contains(A) || !IndexRemap.Contains(B) || !IndexRemap.Contains(C))  continue; 
 
        Indices.Add(IndexRemap[A]);
        Indices.Add(IndexRemap[B]);
        Indices.Add(IndexRemap[C]);
    }*/

    if (!ProcMesh) return;
    if (IsEngineExitRequested()) return;
    ProcMesh->CreateMeshSection_LinearColor(0, Vertices, Indices, Normals, UVs, VertexColors, Tangents, true, false);
}

void UVoxelGeneratorComponent::InvokeVoxelRenderer(OctreeNode* node) {
    if (bBufferReady[ReadBufferIndex])
    {
        UpdateMesh(marchingCubesOutBuffer[ReadBufferIndex]);
        bBufferReady[ReadBufferIndex] = false;
        SwapBuffers();
    }

    FMarchingCubesDispatchParams Params(1, 1, 1);
    Params.Input.baseDepthScale = 3.0f;
    Params.Input.isoLevel = 0.5f;
    Params.Input.voxelsPerAxis = voxelsPerAxis;
    Params.Input.tree = node;
    Params.Input.leafCount = leafCount;

    int32 readBufferIndex = ReadBufferIndex;
    FMarchingCubesInterface::Dispatch(Params,
        [WeakThis = TWeakObjectPtr<UVoxelGeneratorComponent>(this), readBufferIndex](FMarchingCubesOutput OutputVal) {
            if (!WeakThis.IsValid()) return;
            if (IsEngineExitRequested()) return;

            WeakThis->bBufferReady[readBufferIndex] = true;
            WeakThis->marchingCubesOutBuffer[readBufferIndex] = OutputVal;
        });


    /*FMarchingCubesInterface::Dispatch(Params, [this](FMarchingCubesOutput OutputVal) {
        if (IsEngineExitRequested()) return;
        bBufferReady[ReadBufferIndex] = true;
        marchingCubesOutBuffer[ReadBufferIndex] = OutputVal;
     });*/
}

void UVoxelGeneratorComponent::TraverseAndDraw(OctreeNode* node) {
    if (!node) return;

    if (node->isLeaf) {
        DrawDebugBox(GetWorld(), FVector(node->bounds.Center()), FVector(node->bounds.Extent()), FColor::Green, false, -1.f, 0, 1.f);
        const int resolution = 2;
        FVector3f min = node->bounds.min;
        FVector3f max = node->bounds.max;

        for (int x = 0; x <= resolution; ++x) {
            for (int y = 0; y <= resolution; ++y) {
                for (int z = 0; z <= resolution; ++z) {
                    FVector3f p = FMath::Lerp(min, max, FVector3f(
                        (float)x / resolution,
                        (float)y / resolution,
                        (float)z / resolution
                    ));
                    float v = SampleSDF(p);
                    FColor color = (FMath::Abs(v) < 0.01f) ? FColor::White : (v < 0.f ? FColor::Blue : FColor::Red);
                    DrawDebugPoint(GetWorld(), FVector(p), 5.f, color, false, -1.f);
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
    //SampleExampleComputeShader();
    InvokeVoxelRenderer((*tree).root);
}

