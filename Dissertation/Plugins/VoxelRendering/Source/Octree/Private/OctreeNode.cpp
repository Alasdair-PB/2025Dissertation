#include "OctreeNode.h"
#include "OctreeModule.h"

OctreeNode::OctreeNode(AActor* inTreeActor, const AABB& inBounds, uint32 bufferSize, uint32 inVoxelsPerAxis, int inDepth, int maxDepth) :
    depth(inDepth), voxelsPerAxis(inVoxelsPerAxis), treeActor(inTreeActor), isLeaf(true), bounds(inBounds), transitionCellBufferSize((2 * voxelsPerAxis + 1)* (2 * voxelsPerAxis + 1)),
    regularCell(RegularCell(bufferSize)) {

    vertexFactory = MakeShareable(new FVoxelVertexFactory(bufferSize));
    regularCell = RegularCell(bufferSize);

    uint32 isoValuesPerAxis = inVoxelsPerAxis + 1;
    uint32 transitionCellBufferSize = (2 * voxelsPerAxis + 1) * (2 * voxelsPerAxis + 1);

    for (int i = 0; i < 3; i++) {
        transitonCells[i] = TransitionCell(transitionCellBufferSize);
    }

    ENQUEUE_RENDER_COMMAND(InitVoxelResources)(
        [this, bufferSize, transitionCellBufferSize](FRHICommandListImmediate& RHICmdList)
        {
            vertexFactory->Initialize(bufferSize * 15);
            regularCell.Initialize(bufferSize);

            for (TransitionCell cell : transitonCells)
                cell.Initialize(transitionCellBufferSize);
        });

    for (int i = 0; i < 8; ++i)
        children[i] = nullptr;

    if (inDepth < maxDepth)
        Subdivide(inDepth, maxDepth, bufferSize);
}

 OctreeNode::~OctreeNode() {
    Release();
    FlushRenderingCommands();

    vertexFactory.Reset();
    regularCell.ResetResources();

    for (TransitionCell cell : transitonCells)
        cell.ResetResources();

    for (int i = 0; i < 8; ++i) {
        delete children[i];
        children[i] = nullptr;
    }
}

void OctreeNode::Release() {
    ENQUEUE_RENDER_COMMAND(ReleaseTypeBufferCmd)(
        [this](FRHICommandListImmediate& RHICmdList) {
            if (vertexFactory.IsValid())
                vertexFactory->ReleaseResource();
            regularCell.ReleaseResources();
            for (TransitionCell cell : transitonCells)
                cell.ReleaseResources();
        });
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
        children[i] = new OctreeNode(treeActor, childAABB, bufferSize, voxelsPerAxis, nextDepth, maxDepth);
    }
}