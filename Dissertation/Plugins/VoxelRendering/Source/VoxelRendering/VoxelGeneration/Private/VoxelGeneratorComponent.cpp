#include "VoxelGeneratorComponent.h"
#include "MySimpleComputeShader.h"
#include "Logging/LogMacros.h"

static const float isoLevel = 0.5f;

UVoxelGeneratorComponent::UVoxelGeneratorComponent() {
	PrimaryComponentTick.bCanEverTick = true;
}

void UVoxelGeneratorComponent::BeginPlay()
{
	Super::BeginPlay();
    AActor* Owner = GetOwner();
    voxelRenderer = NewObject<UVoxelRendererComponent>(Owner);
    voxelRenderer->RegisterComponent();
    InitOctree();

    ProcMesh = NewObject<UProceduralMeshComponent>(Owner);
    ProcMesh->RegisterComponent();
    ProcMesh->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
}

void UVoxelGeneratorComponent::BeginDestroy() {
    Super::BeginDestroy();
    //voxelRenderer->DestroyComponent();
    delete stopWatch;
    delete tree;
}

void UVoxelGeneratorComponent::InitOctree() {
    AABB bounds = { FVector3f(-200.0f), FVector3f(200.0f) };
    tree = new Octree(bounds);
    leafCount = GetLeafCount((*tree).root);

    int depth = 2; 
    int nodesPerAxisMaxRes = Octree::IntPow(2, depth);
    int size = voxelsPerAxis * nodesPerAxisMaxRes;

    DispatchIsoBuffer(size, depth);
}

void UVoxelGeneratorComponent::BuildOctree(int size, int depth)
{
    if (!tree->BuildFromBuffers(isovalueBuffer, typeValueBuffer, size, depth))
        UE_LOG(LogTemp, Warning, TEXT("Tree failed to allocate values"));
    leafCount = GetLeafCount((*tree).root);
    // Get vertex count?
    dispathShader = true;
}

void UVoxelGeneratorComponent::DispatchIsoBuffer(int size, int depth) {
    int isoSize = size + 1;
    isovalueBuffer.Reserve(isoSize * isoSize * isoSize);
    typeValueBuffer.Reserve(isoSize * isoSize * isoSize);

    FPlanetGeneratorDispatchParams Params(isoSize, isoSize, isoSize);
    Params.Input.baseDepthScale = 400.0f;
    Params.Input.size = isoSize;
    Params.Input.isoLevel = isoLevel;
    Params.Input.planetScaleRatio = 0.9;
    Params.Input.seed = 0;

    FPlanetGeneratorInterface::Dispatch(Params,
        [WeakThis = TWeakObjectPtr<UVoxelGeneratorComponent>(this), size, depth](FPlanetGeneratorOutput OutputVal) {
            if (!WeakThis.IsValid()) return;
            if (IsEngineExitRequested()) return;

            WeakThis->isovalueBuffer = OutputVal.outIsoValues;
            WeakThis->typeValueBuffer = OutputVal.outTypeValues;
            WeakThis->BuildOctree(size, depth);
        });
}

float UVoxelGeneratorComponent::SampleSDF(FVector3f p) {
    return (p.Length()) - 1.0f;
}

void UVoxelGeneratorComponent::SampleExampleComputeShader() {
    FMySimpleComputeShaderDispatchParams Params(1, 1, 1);
    Params.Input[0] = 2;
    Params.Input[1] = 5;

    FMySimpleComputeShaderInterface::Dispatch(Params, [](int OutputVal) {
        UE_LOG(LogTemp, Warning, TEXT("This is a debug message with value: %d"), OutputVal);});
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

void DrawProcMeshEdges(UProceduralMeshComponent* ProcMesh, TArray<FVector>& Vertices, TArray<int32>& Indices, float Duration = 5.0f, FColor LineColor = FColor::Green)
{
    double x= FPlatformTime::Seconds();
    for (int32 i = 0; i < Indices.Num(); i += 3)
    {
        FVector V0 = Vertices[Indices[i]];
        FVector V1 = Vertices[Indices[i + 1]];
        FVector V2 = Vertices[Indices[i + 2]];

        DrawDebugLine(ProcMesh->GetWorld(), V0, V1, LineColor, false, Duration, 0, 1.0f);
        DrawDebugLine(ProcMesh->GetWorld(), V1, V2, LineColor, false, Duration, 0, 1.0f);
        DrawDebugLine(ProcMesh->GetWorld(), V2, V0, LineColor, false, Duration, 0, 1.0f);
    }
}

void AddVertice(TArray<FVector>& Vertices, TArray<int32>& Indices, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FProcMeshTangent>& Tangents,
    TArray<FLinearColor>& VertexColors, TMap<int32, int32>& IndexRemap, int outVertIndex, int* includedVertIndex, FMarchingCubesOutput& meshInfo){
    FVector V = FVector(meshInfo.outVertices[outVertIndex]);
    FVector N = FVector(meshInfo.outNormals[outVertIndex]);
    IndexRemap.Add(outVertIndex, *includedVertIndex);
    Vertices.Add(V);
    Normals.Add(N);
    UVs.Add(FVector2D(0, 0));
    Tangents.Add(FProcMeshTangent(1, 0, 0));
    VertexColors.Add(FColor::White);
    (*includedVertIndex)++;
}

const bool DebugVoxelMesh = false;

void UVoxelGeneratorComponent::UpdateMesh(FMarchingCubesOutput meshInfo) {
    TArray<FVector> Vertices;
    TArray<int32> Indices;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;
    TArray<FLinearColor> VertexColours;
    TMap<int32, int32> IndexRemap;
    int32 includedVertIndex = 0;

    for (int32 i = 0; i < meshInfo.outTris.Num(); i += 3) {
        int32 A = meshInfo.outTris[i];
        int32 B = meshInfo.outTris[i + 1];
        int32 C = meshInfo.outTris[i + 2];

        if (A == -1 || B == -1 || C == -1) { continue; }

        AddVertice(Vertices, Indices, Normals, UVs, Tangents, VertexColours, IndexRemap, i, &includedVertIndex, meshInfo);
        AddVertice(Vertices, Indices, Normals, UVs, Tangents, VertexColours, IndexRemap, i + 1, &includedVertIndex, meshInfo);
        AddVertice(Vertices, Indices, Normals, UVs, Tangents, VertexColours, IndexRemap, i + 2, &includedVertIndex, meshInfo);

        if (!IndexRemap.Contains(A) || !IndexRemap.Contains(B) || !IndexRemap.Contains(C)) { 
            UE_LOG(LogTemp, Warning, TEXT("Too few vertices have been created for this triangle index to be valid: %d, %d, %d"), A, B, C);
            continue; 
        }

        int32 IA = IndexRemap[A];
        int32 IB = IndexRemap[B];
        int32 IC = IndexRemap[C];

        if (IA == IB || IB == IC || IC == IA) {
            UE_LOG(LogTemp, Warning, TEXT("Bad triangle IA == IB || IB == IC || IC == IA : %d"), A);
            continue;
        }
        if (Vertices[IA] == Vertices[IB] || Vertices[IB] == Vertices[IC] || Vertices[IC] == Vertices[IA]) {
            UE_LOG(LogTemp, Warning, TEXT("Bad triangle, vertices[IA] == Vertices[IB]: %f %f %f"), Vertices[IA].X, Vertices[IA].Y, Vertices[IA].Z);
            UE_LOG(LogTemp, Warning, TEXT("Bad triangle, vertices[IA] == Vertices[IB]: %f %f %f"), Vertices[IB].X, Vertices[IB].Y, Vertices[IB].Z);
            UE_LOG(LogTemp, Warning, TEXT("Bad triangle, vertices[IA] == Vertices[IB]: %f %f %f"), Vertices[IC].X, Vertices[IC].Y, Vertices[IC].Z);
            UE_LOG(LogTemp, Warning, TEXT("More debug info: %d %d %d"), IA, IB, IC);
            break;
        }

        Indices.Add(IA);
        Indices.Add(IB);
        Indices.Add(IC);
    }

    if (!ProcMesh) return;
    if (IsEngineExitRequested()) return;

    ProcMesh->CreateMeshSection_LinearColor(0, Vertices, Indices, Normals, UVs, VertexColours, Tangents, true, false);
    if (DebugVoxelMesh) DrawProcMeshEdges(ProcMesh, Vertices, Indices);
}

void UVoxelGeneratorComponent::InvokeVoxelRenderer(OctreeNode* node) {
    if (bBufferReady[ReadBufferIndex])
    {
        UpdateMesh(marchingCubesOutBuffer[ReadBufferIndex]);
        bBufferReady[ReadBufferIndex] = false;
        SwapBuffers();
    }

    if (dispathShader) {
        FMarchingCubesDispatchParams Params(1, 1, 1);
        Params.Input.baseDepthScale = 400.0f;
        Params.Input.isoLevel = isoLevel;
        Params.Input.voxelsPerAxis = voxelsPerAxis;
        Params.Input.tree = node;
        Params.Input.leafCount = leafCount;

        int32 readBufferIndex = ReadBufferIndex;
        double password = 0.0;
        stopWatch->TryStartSecureStopWatch(password);
        dispathShader = false;
        FMarchingCubesInterface::Dispatch(Params,
            [WeakThis = TWeakObjectPtr<UVoxelGeneratorComponent>(this), readBufferIndex, password](FMarchingCubesOutput OutputVal) {
                if (!WeakThis.IsValid()) return;
                if (IsEngineExitRequested()) return;

                WeakThis->bBufferReady[readBufferIndex] = true;
                WeakThis->marchingCubesOutBuffer[readBufferIndex] = OutputVal;

                double outVal;
                if (WeakThis->stopWatch->TryGetSecureMeasurement(outVal, password)) {
                    // UE_LOG(LogTemp, Warning, TEXT("Stop watch thread dispatch out : %f"), outVal);
                    WeakThis->stopWatch->ResetSecureStopWatch(password);
                }
            });
    }
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

