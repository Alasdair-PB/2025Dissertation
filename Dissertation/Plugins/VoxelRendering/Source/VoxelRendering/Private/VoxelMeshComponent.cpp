#include "VoxelMeshComponent.h"
#include "FVoxelVertexFactory.h"
#include "FVoxelSceneProxy.h"
#include "FVoxelVertexFactoryShaderParameters.h"
#include "MarchingCubesDispatcher.h"
#include "MaterialDomain.h"
#include "VoxelWorldSubsystem.h"
#include "RenderData.h"

UVoxelMeshComponent::UVoxelMeshComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bUseAsOccluder = false;

    static ConstructorHelpers::FObjectFinder<UMaterial> MaterialAsset(TEXT("/Game/Materials/M_VoxelPixelShader"));
    if (MaterialAsset.Succeeded()) {
        Material = UMaterialInstanceDynamic::Create(MaterialAsset.Object, this);
        SetMaterial(0, Material);
    }
    else {
        Material = UMaterial::GetDefaultMaterial(MD_Surface);
        SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
    }
}

void UVoxelMeshComponent::OnRegister()
{
    Super::OnRegister();
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
}

UBodySetup* UVoxelMeshComponent::GetBodySetup() {
    return UMeshComponent::GetBodySetup();
}
void UVoxelMeshComponent::SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial) {
    UMeshComponent::SetMaterial(ElementIndex, InMaterial);
}

void UVoxelMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!tree) return;
    TraverseAndDraw(tree->GetRoot());
    InvokeVoxelRenderPasses();
}
void UVoxelMeshComponent::InitVoxelMesh(float scale, int inBufferSizePerAxis, int depth, int voxelsPerAxis, 
    TArray<float>& in_isoValueBuffer, TArray<uint32>& in_typeValueBuffer) 
{    tree = new Octree(isoLevel, scale, voxelsPerAxis, depth, inBufferSizePerAxis, in_isoValueBuffer, in_typeValueBuffer);
}

FPrimitiveSceneProxy* UVoxelMeshComponent::CreateSceneProxy()
{
    sceneProxy = new FVoxelSceneProxy(this);
    return sceneProxy;
}

FBoxSphereBounds UVoxelMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
    if (tree) return tree->CalcVoxelBounds(LocalToWorld);
    return FBoxSphereBounds(FBox(FVector(-200), FVector(200)));
    //return UMeshComponent::CalcBounds(LocalToWorld);
}

float UVoxelMeshComponent::SampleSDF(FVector3f p) {
    return (p.Length()) - 1.0f;
}

// Move to LOD Manager in the future
void UVoxelMeshComponent::SetRenderDataLOD() 
{
    TArray<OctreeNode*> visibleNodes;
    GetVisibleNodes(visibleNodes, tree->GetRoot());

    TArray<FVoxelComputeUpdateNodeData> computeUpdateDataNodes;
    TArray<FVoxelProxyUpdateDataNode> proxyNodes;

    for (OctreeNode* node : visibleNodes)
    {
        uint8 nodeDepth = node->GetDepth();
        FVoxelProxyUpdateDataNode proxyNode(nodeDepth, node);
        FVoxelComputeUpdateNodeData computeUpdateDataNode(node);

        if (proxyNode.BuildDataCache())
            proxyNodes.Emplace(proxyNode);
        if (computeUpdateDataNode.BuildDataCache())
            computeUpdateDataNodes.Emplace(computeUpdateDataNode);
    }
    sceneProxy->UpdateSelectedNodes(proxyNodes);
    InvokeVoxelRenderer(computeUpdateDataNodes);
}

void UVoxelMeshComponent::GetVisibleNodes(TArray<OctreeNode*>& nodes, OctreeNode* node) {
    if (!node) return;

    if (node->IsLeaf()) 
        nodes.Add(node);
    else {
        AABB bounds = node->GetBounds();
        float visibleDistance = 50.0f / (1 << node->GetDepth());
        FVector boundsCenter = FVector(bounds.Center().X, bounds.Center().Y, bounds.Center().Z);
        float distance = (GetComponentLocation() - boundsCenter).Length();

        if (distance < visibleDistance) {
            for (int i = 0; i < 8; i++)
                GetVisibleNodes(nodes, node->children[i]);
        }
        else nodes.Add(node);
    }
}

void UVoxelMeshComponent::InvokeVoxelRenderPasses() {
    if (!sceneProxy) return;
    if (!sceneProxy->IsInitialized()) return;

    SetRenderDataLOD();
}

void UVoxelMeshComponent::InvokeVoxelRenderer(TArray<FVoxelComputeUpdateNodeData>& updateData) {

    if (!sceneProxy) return;
    if (!sceneProxy->IsInitialized()) return;

    FMarchingCubesDispatchParams Params(1, 1, 1);
    Params.Input.updateData = { tree };
    Params.Input.updateData.nodeData = updateData;

    if (!Params.Input.updateData.BuildDataCache())
        return;

    check(Params.Input.updateData.isoBuffer);

    FMarchingCubesInterface::Dispatch(Params,
        [WeakThis = TWeakObjectPtr<UVoxelMeshComponent>(this)](FMarchingCubesOutput OutputVal) {
            if (!WeakThis.IsValid()) return;
            if (IsEngineExitRequested()) return;
        });
}

void UVoxelMeshComponent::TraverseAndDraw(OctreeNode* node) {
    if (!node) return;

    AABB bounds = node->GetBounds();
    DrawDebugBox(GetWorld(), FVector(bounds.Center()), FVector(bounds.Extent()), FColor::Green, false, -1.f, 0, 1.f);

    if (node->IsLeaf()) {

        const int resolution = 2;
        FVector3f min = bounds.min;
        FVector3f max = bounds.max;

        for (int x = 0; x <= resolution; ++x) {
            for (int y = 0; y <= resolution; ++y) {
                for (int z = 0; z <= resolution; ++z) {
                    FVector3f p = FMath::Lerp(min, max, FVector3f(
                        (float) x / resolution,
                        (float) y / resolution,
                        (float) z / resolution
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