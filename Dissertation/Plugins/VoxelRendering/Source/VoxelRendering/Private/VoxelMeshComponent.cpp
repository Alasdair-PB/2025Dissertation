#include "VoxelMeshComponent.h"
#include "FVoxelVertexFactory.h"
#include "FVoxelSceneProxy.h"
#include "FVoxelVertexFactoryShaderParameters.h"
#include "MarchingCubesDispatcher.h"

UVoxelMeshComponent::UVoxelMeshComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bUseAsOccluder = false;

    static ConstructorHelpers::FObjectFinder<UMaterial> MaterialAsset(TEXT("/Game/Materials/M_VoxelPixelShader"));

    if (MaterialAsset.Succeeded()) {
        MaterialInstance = UMaterialInstanceDynamic::Create(MaterialAsset.Object, this);
        SetMaterial(0, MaterialInstance);
    }
}

void UVoxelMeshComponent::BeginPlay() {
    Super::BeginPlay();
}
void UVoxelMeshComponent::BeginDestroy() {
    delete tree;
    Super::BeginDestroy();
}

void UVoxelMeshComponent::PostLoad() {
    Super::PostLoad();
    //UpdateMaterial();
}

UBodySetup* UVoxelMeshComponent::GetBodySetup() {
    return UMeshComponent::GetBodySetup();
}
void  UVoxelMeshComponent::SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial) {
    UMeshComponent::SetMaterial(ElementIndex, InMaterial);
}

void UVoxelMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!tree) return;
    TraverseAndDraw(tree->root);
    InvokeVoxelRenderer(tree->root);
}

void UVoxelMeshComponent::InitVoxelMesh(AABB inBounds, int depth, TArray<float>& inIsovalueBuffer, TArray<uint32>& inTypeValueBuffer) {
    int nodesPerAxisMaxRes = Octree::IntPow(2, depth);
    int size = voxelsPerAxis * nodesPerAxisMaxRes;
    typeValueBuffer = inTypeValueBuffer;
    isoValueBuffer = inIsovalueBuffer;
    BuildOctree(inBounds, size, depth);
}

void UVoxelMeshComponent::BuildOctree(AABB inBounds, int size, int depth)
{
    tree = new Octree(inBounds);
    bounds = inBounds;
    if (!tree->BuildFromBuffers(isoValueBuffer, typeValueBuffer, size, depth))
        UE_LOG(LogTemp, Warning, TEXT("Tree failed to allocate values"));
}

FPrimitiveSceneProxy* UVoxelMeshComponent::CreateSceneProxy()
{
    if (!MaterialInstance)
        return nullptr;
    sceneProxy = new FVoxelSceneProxy(this);
	return sceneProxy; //, GetWorld()->GetFeatureLevel()
}

FBoxSphereBounds UVoxelMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
    return FBoxSphereBounds(FBox(FVector(-100), FVector(100))); // Replace with actual bounds
}

float UVoxelMeshComponent::SampleSDF(FVector3f p) {
    return (p.Length()) - 1.0f;
}

void UVoxelMeshComponent::InvokeVoxelRenderer(OctreeNode* node) {

    FMarchingCubesDispatchParams Params(1, 1, 1);
    FVoxelVertexFactory* vf = sceneProxy->GetVertexFactory();

    FVoxelComputeShaderDispatchData vertexDispatchBuffer = FVoxelComputeShaderDispatchData(
        vf->GetVertexUAV(),
        vf->GetVertexBufferElementsCount(),
        vf->GetVertexBufferBytesPerElement());

    Params.Input.baseDepthScale = 400.0f;
    Params.Input.isoLevel = isoLevel;
    Params.Input.voxelsPerAxis = voxelsPerAxis;
    Params.Input.tree = node;
    Params.Input.vertexBufferRHIRef = vertexDispatchBuffer;
    Params.Input.leafCount = tree->GetLeafCount(); // Note this may not need to be done every dispatch

    FMarchingCubesInterface::Dispatch(Params,
        [WeakThis = TWeakObjectPtr<UVoxelMeshComponent>(this)](FMarchingCubesOutput OutputVal) {
            if (!WeakThis.IsValid()) return;
            if (IsEngineExitRequested()) return;
        });
}

void UVoxelMeshComponent::TraverseAndDraw(OctreeNode* node) {
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