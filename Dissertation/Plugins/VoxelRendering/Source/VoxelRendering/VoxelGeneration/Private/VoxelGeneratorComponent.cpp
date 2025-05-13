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
    tree = new Octree(bounds);

    int32 depth = 2; 
    int32 nodesPerAxisMaxRes = Octree::IntPow(2, depth);
    int32 size = voxelsPerAxis * nodesPerAxisMaxRes;
    TArray<float> isovalueBuffer;
    TArray<uint8> typeBuffer;

    typeBuffer.Reserve(size * size * size);
    isovalueBuffer.Reserve((size + 1) * (size + 1) * (size + 1));

    for (int32 z = 0; z < size; ++z) {
        for (int32 y = 0; y < size; ++y) {
            for (int32 x = 0; x < size; ++x) {
                uint8 type = (x + y + z) % 2;
                typeBuffer.Add(type);
            }
        }
    }
    for (int32 z = 0; z <= size; ++z) {
        for (int32 y = 0; y <= size; ++y) {
            for (int32 x = 0; x <= size; ++x) {
                float iso = (x + y + z) % 2 == 0 ? -1.0f : 1.0f;
                isovalueBuffer.Add(iso);
            }
        }
    }

    if (!tree->BuildFromBuffers(isovalueBuffer, typeBuffer, size, depth)) {
        UE_LOG(LogTemp, Warning, TEXT("Tree failed to allocate values"));
    }
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

void AddVertice(TArray<FVector>& Vertices, TArray<int32>& Indices, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FProcMeshTangent>& Tangents,
    TArray<FLinearColor>& VertexColors, TMap<int32, int32>& IndexRemap, int i, int& includedVertIndex, FMarchingCubesOutput& meshInfo)
{
    FVector V = FVector(meshInfo.outVertices[i]);
    FVector N = FVector(meshInfo.outNormals[i]);

    IndexRemap.Add(i, includedVertIndex);
    Vertices.Add(V);
    Normals.Add(N);
    UVs.Add(FVector2D(0, 0));
    Tangents.Add(FProcMeshTangent(1, 0, 0));
    VertexColors.Add(FColor::White);
    includedVertIndex++;
}

void UVoxelGeneratorComponent::UpdateMesh(FMarchingCubesOutput& meshInfo) {
    TArray<FVector> Vertices;
    TArray<int32> Indices;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;
    TArray<FLinearColor> VertexColours;
    TMap<int32, int32> IndexRemap;

    int32 includedVertIndex = 0;

    for (int32 i = 0; i < meshInfo.outVertices.Num(); i++) {
        FVector V = FVector(meshInfo.outVertices[i]);
        FVector N = FVector(meshInfo.outNormals[i]);
        if (V == FVector(-1, -1, -1)) continue; // Need a different null value indicator as a vector of -1,-1,-1 can exist

        IndexRemap.Add(i, includedVertIndex);
        Vertices.Add(V);
        Normals.Add(N);
        UVs.Add(FVector2D(0, 0));
        Tangents.Add(FProcMeshTangent(1, 0, 0));
        VertexColours.Add(FColor::White);
        includedVertIndex++;
    }

    for (int32 i = 0; i < meshInfo.outTris.Num(); i += 3) {
        int32 A = meshInfo.outTris[i];
        int32 B = meshInfo.outTris[i + 1];
        int32 C = meshInfo.outTris[i + 2];

        if (A == -1 || B == -1 || C == -1) continue;

        //if (!IndexRemap.Contains(A) || !IndexRemap.Contains(B) || !IndexRemap.Contains(C)) { 
        //    UE_LOG(LogTemp, Warning, TEXT("Too few vertices have been created for this triangle index to be valid: %d, %d, %d"), A, B, C);
        //    continue; 
        //}

        int32 IA = IndexRemap[A];
        int32 IB = IndexRemap[B];
        int32 IC = IndexRemap[C];

        if (IA == IB || IB == IC || IC == IA) {
            UE_LOG(LogTemp, Warning, TEXT("Bad triangle IA == IB || IB == IC || IC == IA : %d"), A);
            continue;
        }
        if (Vertices[IA] == Vertices[IB] || Vertices[IB] == Vertices[IC] || Vertices[IC] == Vertices[IA]) {
            UE_LOG(LogTemp, Warning, TEXT("Bad triangle, vertices[IA] == Vertices[IB]: %d"), A);
            continue;
        }

        Indices.Add(IA);
        Indices.Add(IB);
        Indices.Add(IC);
    }

    if (!ProcMesh) return;
    if (IsEngineExitRequested()) return;
    ProcMesh->CreateMeshSection_LinearColor(0, Vertices, Indices, Normals, UVs, VertexColours, Tangents, true, false);
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

