#include "OctreeNode.h"
#include "OctreeModule.h"

OctreeNode::OctreeNode(const AABB& inBounds, uint32 bufferSize, int inDepth, int maxDepth) : depth(inDepth), isLeaf(true) {
    vertexFactory = MakeShareable(new FVoxelVertexFactory(bufferSize));
    avgIsoBuffer = MakeShareable(new FIsoDynamicBuffer(bufferSize));
    avgTypeBuffer = MakeShareable(new FTypeDynamicBuffer(bufferSize));

    ENQUEUE_RENDER_COMMAND(InitVoxelResources)(
        [this, bufferSize](FRHICommandListImmediate& RHICmdList)
        {
            vertexFactory->Initialize(bufferSize);
            avgTypeBuffer->Initialize(bufferSize);
            avgTypeBuffer->Initialize(bufferSize);
        });
    for (int i = 0; i < 8; ++i)
        children[i] = nullptr;

    if (depth < maxDepth)
        Subdivide(maxDepth, bufferSize);
}

OctreeNode::~OctreeNode() {
	Release();
	for (int i = 0; i < 8; ++i)
		delete children[i];
}

void OctreeNode::Release() {
	avgIsoBuffer->ReleaseResource();
	avgTypeBuffer->ReleaseResource();
}

void OctreeNode::Subdivide(int maxDepth, int bufferSize) {
    if (!isLeaf) return;

    int nextDepth = depth + 1;
    isLeaf = false;
    FVector3f c = bounds.Center();
    FVector3f e = bounds.Extent() * 0.5f;

    for (int i = 0; i < 8; ++i) {
        FVector3f offset((i & 1 ? 1 : -1) * e.X, (i & 2 ? 1 : -1) * e.Y, (i & 4 ? 1 : -1) * e.Z);
        FVector3f childCenter = c + offset;
        FVector3f halfSize = e;

        AABB childAABB = { childCenter - halfSize,childCenter + halfSize };
        children[i] = new OctreeNode(childAABB, bufferSize, nextDepth, maxDepth);
    }
}