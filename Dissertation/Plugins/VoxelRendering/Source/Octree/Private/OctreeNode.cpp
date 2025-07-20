#include "OctreeNode.h"
#include "OctreeModule.h"

OctreeNode::OctreeNode(AActor* inTreeActor, const AABB& inBounds, uint32 bufferSize, int inDepth, int maxDepth) : 
    depth(inDepth), isLeaf(true), bounds(inBounds), treeActor(inTreeActor) {
    vertexFactory = MakeShareable(new FVoxelVertexFactory(bufferSize));
    avgIsoBuffer = MakeShareable(new FIsoDynamicBuffer(bufferSize));
    avgTypeBuffer = MakeShareable(new FTypeDynamicBuffer(bufferSize));

    ENQUEUE_RENDER_COMMAND(InitVoxelResources)(
        [this, bufferSize](FRHICommandListImmediate& RHICmdList)
        {
            vertexFactory->Initialize(bufferSize * 15);
            avgIsoBuffer->Initialize(bufferSize);
            avgTypeBuffer->Initialize(bufferSize);
        });

    for (int i = 0; i < 8; ++i)
        children[i] = nullptr;

    if (inDepth < maxDepth)
        Subdivide(inDepth, maxDepth, bufferSize);
}

 OctreeNode::~OctreeNode() {
    Release();
    FlushRenderingCommands();
    avgIsoBuffer.Reset();
    avgTypeBuffer.Reset();
    vertexFactory.Reset();
    for (int i = 0; i < 8; ++i) {
        delete children[i];
        children[i] = nullptr;
    }
}

void OctreeNode::Release() {

    if (avgIsoBuffer.IsValid()) {
        ENQUEUE_RENDER_COMMAND(ReleaseIsoBufferCmd)(
            [avgIsoBuffer = avgIsoBuffer](FRHICommandListImmediate& RHICmdList) {
                avgIsoBuffer->ReleaseResource();
            });
    }

    if (avgTypeBuffer.IsValid()) {
        ENQUEUE_RENDER_COMMAND(ReleaseTypeBufferCmd)(
            [avgTypeBuffer = avgTypeBuffer](FRHICommandListImmediate& RHICmdList) {
                avgTypeBuffer->ReleaseResource();
            });
    }

    if (vertexFactory.IsValid()) {
        ENQUEUE_RENDER_COMMAND(ReleaseTypeBufferCmd)(
            [vertexFactory = vertexFactory](FRHICommandListImmediate& RHICmdList) {
                vertexFactory->ReleaseResource();
            });
    }
}

void OctreeNode::Subdivide(int inDepth, int maxDepth, int bufferSize) {
    if (!isLeaf) return;

    int nextDepth = inDepth + 1;
    isLeaf = false;
    FVector3f c = bounds.Center();
    FVector3f e = bounds.Extent() * 0.5f;

    for (int i = 0; i < 8; ++i) {
        FVector3f offset((i & 1 ? 1 : -1) * e.X, (i & 2 ? 1 : -1) * e.Y, (i & 4 ? 1 : -1) * e.Z);
        FVector3f childCenter = c + offset;
        FVector3f halfSize = e;

        AABB childAABB = { childCenter - halfSize, childCenter + halfSize };
        children[i] = new OctreeNode(treeActor, childAABB, bufferSize, nextDepth, maxDepth);
    }
}