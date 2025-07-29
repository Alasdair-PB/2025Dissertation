#include "VoxelMeshComponent.h"
#include "FVoxelVertexFactory.h"
#include "FVoxelSceneProxy.h"
#include "FVoxelVertexFactoryShaderParameters.h"
#include "MarchingCubesDispatcher.h"
#include "MaterialDomain.h"
#include "VoxelWorldSubsystem.h"
#include "RenderData.h"
#include "PhysicsEngine/BodySetup.h"

UVoxelMeshComponent::UVoxelMeshComponent() : voxelBodySetup(nullptr)
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

void UVoxelMeshComponent::CheckVoxelMining() {
    if (!playerController)
        playerController = GetWorld()->GetFirstPlayerController();

    bool leftMouseDown = playerController->IsInputKeyDown(EKeys::LeftMouseButton);
    bool keyDown = leftMouseDown || playerController->IsInputKeyDown(EKeys::RightMouseButton);

    if (!keyDown || playerController->IsInputKeyDown(EKeys::LeftShift)) return;

    FVector worldLoc, worldDir;
    if (playerController->DeprojectMousePositionToWorld(worldLoc, worldDir))
    {
        FHitResult hit;
        FVector start = worldLoc;
        FVector end = start + worldDir * 1000;

        if (tree->RaycastToVoxelBody(hit, start, end))
        {
            FVector position = hit.Location;
            leftMouseDown ? 
            tree->ApplyDeformationAtPosition(position, 30.0f, 1.0f) :
            tree->ApplyDeformationAtPosition(position, 30.0f, 1.0f, 1, true);
        }
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

void UVoxelMeshComponent::SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial) {
    UMeshComponent::SetMaterial(ElementIndex, InMaterial);
}

const bool rotatePlanet = false;
void UVoxelMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!tree) return;
    TraverseAndDraw(tree->GetRoot());
    InvokeVoxelRenderPasses();
    if (rotatePlanet) RotateAroundAxis(FVector(0.2f, 1.0f, 0.2f), 0.5f);
}

void UVoxelMeshComponent::RotateAroundAxis(FVector axis, float degreeTick)
{
    axis = axis.GetSafeNormal();
    FQuat quatRotation = FQuat(axis, FMath::DegreesToRadians(degreeTick));
    FQuat currentRotation = GetComponentQuat();
    FQuat newRotation = quatRotation * currentRotation;
    newRotation.Normalize();
    GetOwner()->SetActorRotation(newRotation);
}

void UVoxelMeshComponent::InitVoxelMesh(
    float scale, int inBufferSizePerAxis, int depth, int voxelsPerAxis, 
    TArray<float>& in_isoValueBuffer, TArray<uint32>& in_typeValueBuffer,
    AActor* inEraser, AActor* inPlayer)
{    
    eraser = inEraser;
    player = inPlayer;

    AActor* owner = GetOwner();
    tree = new Octree(owner, isoLevel, scale, voxelsPerAxis, depth, inBufferSizePerAxis, in_isoValueBuffer, in_typeValueBuffer);
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
}

UBodySetup* UVoxelMeshComponent::GetBodySetup() {
    return voxelBodySetup;
}

void ResetVisibleNodes(OctreeNode* node) {
    if (!node) return;
    node->SetVisible(false);

    for (int i = 0; i < 8; ++i)
        ResetVisibleNodes(node->children[i]);
}

void UVoxelMeshComponent::SetRenderDataLOD() 
{
    TArray<OctreeNode*> visibleNodes;

    ResetVisibleNodes(tree->GetRoot());
    GetVisibleNodes(visibleNodes, tree->GetRoot());
    BalanceVisibleNodes(visibleNodes);

    TArray<FVoxelComputeUpdateNodeData> computeUpdateDataNodes;
    TArray<FVoxelProxyUpdateDataNode> proxyNodes;
    TArray<FVoxelTransVoxelNodeData> computeTransvoxelData;

    for (OctreeNode* node : visibleNodes)
    {
        uint8 nodeDepth = node->GetDepth();
        FVoxelProxyUpdateDataNode proxyNode(nodeDepth, node);
        FVoxelComputeUpdateNodeData computeUpdateDataNode(node);

        if (proxyNode.BuildDataCache())
            proxyNodes.Emplace(proxyNode);
        if (computeUpdateDataNode.BuildDataCache())
            computeUpdateDataNodes.Emplace(computeUpdateDataNode);

        for (int i = 0; i < 3; i++) {
            TransitionCell* cell = nullptr;
            cell = node->GetTransitionCell(i);
            if (cell && cell->enabled) {
                if (cell->adjacentNodeIndex != 4) {
                    UE_LOG(LogTemp, Warning, TEXT("Debug: cell has non-3 index: %d"), cell->adjacentNodeIndex);
                    continue;
                }
                FVoxelTransVoxelNodeData nodeData(cell, node, i);
                nodeData.BuildDataCache();
                computeTransvoxelData.Add(nodeData);
            }
            else {
                FVoxelTransVoxelNodeData nodeData(node, i);
                nodeData.BuildDataCache();
                computeTransvoxelData.Add(nodeData);
            }
        }
    }
    sceneProxy->UpdateSelectedNodes(proxyNodes);
    InvokeVoxelRenderer(computeUpdateDataNodes, computeTransvoxelData);
}

void UVoxelMeshComponent::SetNodeVisible(TArray<OctreeNode*>& nodes, OctreeNode* node) {
    node->SetVisible(true);
    nodes.Add(node);
}

void UVoxelMeshComponent::SetChildrenVisible(TArray<OctreeNode*>& pushStack, OctreeNode* node, int currentDepth, int targetDepth) {
    if (!node) return;

    if (currentDepth >= targetDepth || node->IsLeaf()) {
        pushStack.Add(node);
        return;
    }

    for (int l = 0; l < 8; l++)
        SetChildrenVisible(pushStack, node->children[l], (currentDepth + 1), targetDepth);
}

bool AreAdjacent(OctreeNode* a, OctreeNode* b, const FIntVector& direction) {
    const float epsilon = 0.0001f;
    const AABB& boundsA = a->GetBounds();
    const AABB& boundsB = b->GetBounds();

    for (int axis = 0; axis < 3; axis++) {
        if (FMath::Abs(direction[axis]) > 0.5f) {
            float faceA = direction[axis] > 0 ? boundsA.max[axis] : boundsA.min[axis];
            float faceB = direction[axis] > 0 ? boundsB.min[axis] : boundsB.max[axis];

            if (FMath::Abs(faceA - faceB) > epsilon) // faceA != faceB
                return false;
        }
        else {
            if (boundsA.max[axis] <= boundsB.min[axis] + epsilon || boundsA.min[axis] >= boundsB.max[axis] - epsilon)
                return false;
        }
    }
    return true;
}

void GetNodesAdjacent(TArray<OctreeNode*>& adjNodes, OctreeNode* node, TArray<OctreeNode*> visibleNodes, FIntVector offset) {
    FVector nodeScale = node->GetNodeSize();
    for (OctreeNode* checkNode : visibleNodes) {
        if (checkNode == node) continue;

        if (AreAdjacent(checkNode, node, offset))
            adjNodes.Add(checkNode);
    }
}

int GetAdjacencyIndex(OctreeNode* node, OctreeNode* adjacentNode, int offsetIndex)
{
    FIntVector offset = neighborOffsets[offsetIndex];
    const float epsilon = 0.0001f;

    const AABB& boundsA = adjacentNode->GetBounds();
    const AABB& boundsB = node->GetBounds();

    int mainAxis = (offset.X != 0) ? 0 : (offset.Y != 0) ? 1 : 2;

    int axisA = (mainAxis + 1) % 3;
    int axisB = (mainAxis + 2) % 3;

    FIntVector2 index;
    index.X = boundsA.max[axisA] + epsilon >= boundsB.max[axisA] ? 1 : 0;
    index.Y = boundsA.max[axisB] + epsilon >= boundsB.max[axisB] ? 1 : 0;

    int flatIndex = index.X + (index.Y * 2);
    return flatIndex;
}

bool UVoxelMeshComponent::BalanceNode(TArray<OctreeNode*>& removalStack, TArray<OctreeNode*>& pushStack, TArray<OctreeNode*>& visibleNodes, OctreeNode* node) {
    if (node->IsLeaf()) return false;
    node->ResetTransVoxelData();
    FVector nodeSize = node->GetNodeSize();

    int nodeDepth = node->GetDepth();

    for (int i = 0; i < 6; i++) {
        FIntVector offset = neighborOffsets[i];
        TArray<OctreeNode*> neighbors; 
        GetNodesAdjacent(neighbors, node, visibleNodes, offset);

        if (neighbors.Num() == 0)
            continue;

        int deepestAdjacentDepth = 0;
        OctreeNode* deepestAdjacentNode;

        for (OctreeNode* adjNode : neighbors) {
            int nodeDifference = adjNode->GetDepth() - node->GetDepth();

            if (nodeDifference > deepestAdjacentDepth) {
                deepestAdjacentNode = adjNode;
                deepestAdjacentDepth = nodeDifference;
            }

            if (nodeDifference == 1)
                node->AssignTransVoxelData(i, adjNode, GetAdjacencyIndex(node, adjNode, i));
        }
        if (deepestAdjacentDepth < 2) continue;

        SetChildrenVisible(pushStack, node, 0, deepestAdjacentDepth - 1);
        node->ResetTransVoxelData();
        node->SetVisible(false);
        removalStack.Add(node);
        return true;
    }
    return false;
}

void UVoxelMeshComponent::BalanceVisibleNodes(TArray<OctreeNode*>& visibleNodes) {
    TArray<OctreeNode*> removalStack;
    TArray<OctreeNode*> pushStack;
    bool reBalance = false;

    for (OctreeNode* node : visibleNodes) {
        reBalance = BalanceNode(removalStack, pushStack, visibleNodes, node);
        if (reBalance) break;
    }

    visibleNodes.RemoveAll([&](OctreeNode* n) {
        return removalStack.Contains(n);
    });

    for (OctreeNode* node : pushStack) {
        if (!visibleNodes.Contains(node))
            visibleNodes.Add(node);
    }

    if (reBalance)
        BalanceVisibleNodes(visibleNodes);
}

const bool b_UseNode = true;
void UVoxelMeshComponent::GetVisibleNodes(TArray<OctreeNode*>& nodes, OctreeNode* node) {
    if (!node) return;

    if (!playerController)
        playerController = GetWorld()->GetFirstPlayerController();

    APawn* playerPawn = playerController->GetPawn();
    FTransform playerTransform = b_UseNode ? player->GetActorTransform() : playerPawn->GetActorTransform();
    FVector playerPos = playerTransform.GetLocation();
    playerPos = GetComponentTransform().InverseTransformPosition(playerPos);

    if (node->IsLeaf()) 
        SetNodeVisible(nodes, node);
    else {
        float visibleDistance = 1000.0f / (1 << node->GetDepth());
        FVector boundsCenter = node->GetWorldNodePosition();
        float distance = (playerPos - boundsCenter).Length();

        if (distance < visibleDistance) {
            for (int i = 0; i < 8; i++)
                GetVisibleNodes(nodes, node->children[i]);
        }
        else SetNodeVisible(nodes, node);
    }
}

void UVoxelMeshComponent::InvokeVoxelRenderPasses() {
    if (!sceneProxy) return;
    if (!sceneProxy->IsInitialized()) return;

    CheckVoxelMining();
    if (eraser) {
        FTransform eraserTransform = eraser->GetActorTransform();
        tree->ApplyDeformationAtPosition(eraserTransform.GetLocation(), 50.0f, 0.5f);
    }
    if (tree->AreValuesDirty()) tree->UpdateValuesDirty();
    SetRenderDataLOD();
}

void UVoxelMeshComponent::InvokeVoxelRenderer(TArray<FVoxelComputeUpdateNodeData>& updateData, TArray<FVoxelTransVoxelNodeData>& transVoxelUpdateData) {

    if (!sceneProxy) return;
    if (!sceneProxy->IsInitialized()) return;

    FMarchingCubesDispatchParams Params(1, 1, 1);
    Params.Input.updateData = { tree };
    Params.Input.updateData.nodeData = updateData;
    Params.Input.updateData.transVoxelNodeData = transVoxelUpdateData;

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

    if (node->IsVisible()) {
        AABB bounds = node->GetBounds();
        DrawDebugBox(GetWorld(), FVector(bounds.Center()), FVector(bounds.Extent()), FColor::Green, false, -1.f, 0, 1.f);
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
                    float v = node->GetDepth();
                    FColor color = (FMath::Abs(v) <= 1.0f) ? FColor::White : (v <= 2.0f ? FColor::Blue : FColor::Red);
                    DrawDebugPoint(GetWorld(), FVector(p), 5.f, color, false, -1.f);
                }
            }
        }
        //return;
    }
    for (int i = 0; i < 8; ++i)
        TraverseAndDraw(node->children[i]);
}
