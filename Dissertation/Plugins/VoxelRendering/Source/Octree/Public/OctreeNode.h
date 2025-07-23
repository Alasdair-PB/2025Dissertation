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

class TransitionCell  {
public:
    TransitionCell() : direction(0), adjacentNodeIndex(0), enabled(false) {}
    int direction; //0-5
    int adjacentNodeIndex;
    bool enabled;

    OctreeNode* adjacentNodes[4]{nullptr, nullptr, nullptr, nullptr};
};

static const TArray<FIntVector> neighborOffsets = {
    FIntVector(-1,  0,  0),
    FIntVector(1,  0,  0),
    FIntVector(0, -1,  0),
    FIntVector(0,  1,  0),
    FIntVector(0,  0, -1),
    FIntVector(0,  0,  1)
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

    void SetTransvoxelDirection(int index, bool state) { 
        if (index < 6 && index >= 0)
            transVoxelDirections[index] = state;
    }

    void ResetTransVoxelData() {
        for (int i = 0; i < 3; i++) {
            transitonCells[i].enabled = false;
            transitonCells[i].adjacentNodeIndex = 0;
            for (int j = 0; j < 4; j++)
                transitonCells[i].adjacentNodes[j] = nullptr;
        }
    }

    void AssignTransVoxelData(int direction, OctreeNode* node) {
        if (direction > 5 && direction < 0) return;
        for (int i = 0; i < 3; i++) {
            if (!(transitonCells[i].enabled)) {
                transitonCells[i].enabled = true;
                transitonCells[i].direction = direction;
                transitonCells[i].adjacentNodeIndex = 0;
                transitonCells[i].adjacentNodes[0] = node;
            }
            else if (transitonCells[i].direction == direction && transitonCells[i].enabled) {
                transitonCells[i].adjacentNodeIndex += 1;
                int newAdjIndex = transitonCells[i].adjacentNodeIndex;
                if (newAdjIndex < 4 && newAdjIndex >= 0)
                    transitonCells[i].adjacentNodes[newAdjIndex] = node;
            }
        }
    }

    void AssignTransVoxelDirections() {
        int transVoxelIndex = 0;
        for (int i = 0; i < 6; i++) {
            bool enabled = false;
            if (transVoxelDirections[i]) {
                if (transVoxelIndex < 3) {
                    transitonCells[transVoxelIndex].direction = i;
                    enabled = true;
                }
                else 
                    UE_LOG(LogTemp, Warning, TEXT("Debug: Attempted to assign more than three transvoxel direction- why?"));
                transVoxelIndex++;
            }
            transitonCells[transVoxelIndex].enabled = enabled;
        }
    }

    OctreeNode* children[8];
    OctreeNode* neighbours[6];
    bool transVoxelDirections[6] {false, false, false, false, false, false};

    void ResetTransvoxelDirections() {
        for (int i = 0; i < 6; ++i) {
            transVoxelDirections[i] = false;
        }
    }

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
