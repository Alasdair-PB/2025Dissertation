#pragma once
#include "CoreMinimal.h"
#include "AABB.h"
#include "FVoxelVertexFactory.h"
#include "RenderResource.h"
#include "VoxelRenderBuffers.h"
#include "Materials/MaterialRelevance.h"

class FVoxelVertexFactory;

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

class TransitionCell : public VoxelCell {
public:
    TransitionCell() : VoxelCell() {}
    TransitionCell(uint32 bufferSize) : VoxelCell(bufferSize) {}
    int direction;
};

class RegularCell : public VoxelCell {
public:
    RegularCell() : VoxelCell() {}
    RegularCell(uint32 bufferSize) : VoxelCell(bufferSize){}
};

class OctreeNode {
public:
    OctreeNode(AActor* treeActor, const AABB& inBounds, uint32 bufferSize, uint32 inVoxelsPerAxis, int depth, int maxDepth);
    ~OctreeNode();

    void Release();
    bool IsLeaf() { return isLeaf; }

    TSharedPtr<FVoxelVertexFactory> GetVertexFactory(){ return vertexFactory; }
    TSharedPtr<FIsoDynamicBuffer> GetIsoBuffer() { return regularCell.avgIsoBuffer; }
    TSharedPtr<FTypeDynamicBuffer> GetTypeBuffer() { return regularCell.avgTypeBuffer; }

    FVector GetNodePosition() const { return FVector(bounds.Center().X, bounds.Center().Y, bounds.Center().Z);  }
    FVector GetWorldNodePosition() const { return  GetNodePosition() + treeActor->GetTransform().GetLocation(); }

    AABB GetBounds() const { return bounds; }
    OctreeNode* children[8];
    int GetDepth() const { return depth; }

protected:
    int maxVertexIndex;
    int depth;
    uint32 voxelsPerAxis;
    AActor* treeActor;
    bool isLeaf;
    bool isVisible;
    AABB bounds;

    uint32 transitionCellBufferSize;

    TSharedPtr<FVoxelVertexFactory> vertexFactory;
    RegularCell regularCell;
    TransitionCell transitonCells[3];

    void Subdivide(int inDepth, int maxDepth, int bufferSize);
};
