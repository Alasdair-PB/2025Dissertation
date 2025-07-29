#pragma once
#include "CoreMinimal.h"
#include "AABB.h"
#include "FVoxelVertexFactory.h"
#include "RenderResource.h"
#include "VoxelRenderBuffers.h"
#include "Materials/MaterialRelevance.h"

class FVoxelVertexFactory;
class OctreeNode;

class VoxelCell {
public:
    TSharedPtr<FIsoDynamicBuffer> avgIsoBuffer;
    TSharedPtr<FTypeDynamicBuffer> avgTypeBuffer;
    VoxelCell() {}

    void Initialize(uint32 bufferSize) {
        avgIsoBuffer->Initialize(bufferSize);
        avgTypeBuffer->Initialize(bufferSize);
    }
    void ReleaseResources() {
        if (avgIsoBuffer.IsValid()) {
            avgIsoBuffer->ReleaseResource();
        }
        if (avgTypeBuffer.IsValid()) {
            avgTypeBuffer->ReleaseResource();
        }
    }

    void ResetResources() {
        avgIsoBuffer.Reset();
        avgTypeBuffer.Reset();
    }

    VoxelCell(uint32 bufferSize) {
        avgIsoBuffer = MakeShareable(new FIsoDynamicBuffer(bufferSize));
        avgTypeBuffer = MakeShareable(new FTypeDynamicBuffer(bufferSize));
    }
};

static const TArray<FIntVector> neighborOffsets = {
    FIntVector(-1,  0,  0),
    FIntVector(1,  0,  0),
    FIntVector(0, -1,  0),
    FIntVector(0,  1,  0),
    FIntVector(0,  0, -1),
    FIntVector(0,  0,  1)
};

class TransitionCell  {
public:
    TransitionCell() : direction(0), adjacentNodeIndex(0), enabled(false) {}
    int direction; //0-5
    int adjacentNodeIndex;
    bool enabled;

    OctreeNode* adjacentNodes[4]{nullptr, nullptr, nullptr, nullptr};
};

class RegularCell : public VoxelCell {
public:
    RegularCell() : VoxelCell() {}
    RegularCell(uint32 bufferSize) : VoxelCell(bufferSize){}
};

class OctreeNode {
public:
    OctreeNode();
    OctreeNode(AActor* treeActor, OctreeNode* inParent, const AABB& inBounds, uint32 bufferSize, uint32 inVoxelsPerAxis, int depth, int maxDepth, bool bInIsRoot = false);
    ~OctreeNode();

    void Release();
    bool IsLeaf() const { return isLeaf; }
    bool IsRoot() const { return isRoot; }
    bool IsVisible() const { return isVisible; }
    void SetVisible(bool visibility) { isVisible = visibility; }

    TSharedPtr<FVoxelVertexFactory> GetVertexFactory(){ return vertexFactory; }
    TSharedPtr<FIsoDynamicBuffer> GetIsoBuffer() { return regularCell.avgIsoBuffer; }
    TSharedPtr<FTypeDynamicBuffer> GetTypeBuffer() { return regularCell.avgTypeBuffer; }
    OctreeNode* GetNodeParent() const { return parent; }

    FVector GetNodePosition() const { return FVector(bounds.Center().X, bounds.Center().Y, bounds.Center().Z);  }
    FVector GetNodeSize() const { return FVector(bounds.Size().X, bounds.Size().Y, bounds.Size().Z); }
    FVector GetWorldNodePosition() const { return  GetNodePosition() + treeActor->GetTransform().GetLocation(); }
    void AssignChildNeighboursAsRoot();
    AABB GetBounds() const { return bounds; }
    int GetDepth() const { return depth; }

    TransitionCell* GetTransitionCell(int index) { 
        if (index < 3 && index >= 0)
            return &transitonCells[index];
        else return nullptr;
    }

    void ResetTransVoxelData() {
        for (int i = 0; i < 3; i++) {
            transitonCells[i].enabled = false;
            transitonCells[i].adjacentNodeIndex = 0;
            for (int j = 0; j < 4; j++)
                transitonCells[i].adjacentNodes[j] = nullptr;
        }
    }

    void AssignTransVoxelData(int inDirection, OctreeNode* inNode) {
        if (inDirection > 5 || inDirection < 0) return;
        for (int i = 0; i < 3; i++) {
            if (!(transitonCells[i].enabled)) {
                transitonCells[i].enabled = true;
                transitonCells[i].direction = inDirection;
                transitonCells[i].adjacentNodeIndex = 1;
                transitonCells[i].adjacentNodes[0] = inNode;
                return;
            }
            else if (transitonCells[i].direction == inDirection && transitonCells[i].enabled) {
                transitonCells[i].enabled = true;
                int newAdjIndex = transitonCells[i].adjacentNodeIndex;
                if (newAdjIndex < 4 && newAdjIndex >= 0) {
                    transitonCells[i].adjacentNodes[newAdjIndex] = inNode;
                    transitonCells[i].adjacentNodeIndex += 1;
                    return;
                }
                UE_LOG(LogTemp, Warning, TEXT("Debug: cell has invalid adjacent index count: %d"), newAdjIndex);
                return;
            }
        }
    }

    OctreeNode* children[8];
    OctreeNode* neighbours[6];

protected:
    int depth;
    uint32 voxelsPerAxis;
    AActor* treeActor;
    OctreeNode* parent;

    bool isLeaf;
    bool isVisible;
    bool isRoot;
    AABB bounds;

    TSharedPtr<FVoxelVertexFactory> vertexFactory;
    RegularCell regularCell;
    TransitionCell transitonCells[3];

    void AssignChildNeighbours(OctreeNode* inNeighbours[6]);
    void Subdivide(int inDepth, int maxDepth, int bufferSize);
};
