#pragma once
#include "CoreMinimal.h"
#include "AABB.h"
#include "FVoxelVertexFactory.h"
#include "RenderResource.h"
#include "VoxelRenderBuffers.h"
#include "Materials/MaterialRelevance.h"

class FVoxelVertexFactory;

class OctreeNode {
public:
    OctreeNode(const AABB& inBounds, uint32 bufferSize, int depth, int maxDepth);
    ~OctreeNode();

    void Release();
    bool IsLeaf() { return isLeaf; }

    TSharedPtr<FVoxelVertexFactory> GetVertexFactory(){ return vertexFactory; }
    TSharedPtr<FIsoDynamicBuffer> GetIsoBuffer() { return avgIsoBuffer; }
    TSharedPtr<FTypeDynamicBuffer> GetTypeBuffer() { return avgTypeBuffer; }

    AABB GetBounds() const { return bounds; }
    OctreeNode* children[8];
    int GetDepth() const { return depth; }

protected:
    int maxVertexIndex;
    int depth;

    bool isLeaf;
    bool isVisible;
    AABB bounds;

    TSharedPtr<FVoxelVertexFactory> vertexFactory;
    TSharedPtr<FIsoDynamicBuffer> avgIsoBuffer;
    TSharedPtr<FTypeDynamicBuffer> avgTypeBuffer;

    void Subdivide(int inDepth, int maxDepth, int bufferSize);
};
