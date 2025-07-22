#include "OctreeNode.h"
#include "OctreeModule.h"

OctreeNode::OctreeNode() :
    neighbours{ nullptr, nullptr, nullptr, nullptr, nullptr,nullptr }, depth(0), voxelsPerAxis(0), treeActor(nullptr),
    parent(nullptr), isLeaf(true), isVisible(false), isRoot(false), bounds(AABB()),
    regularCell(RegularCell(0)) {}

OctreeNode::OctreeNode(AActor* inTreeActor, OctreeNode* inParent, const AABB& inBounds, uint32 bufferSize, uint32 inVoxelsPerAxis, int inDepth, int maxDepth, bool bInIsRoot) :
    neighbours{ nullptr, nullptr, nullptr, nullptr, nullptr,nullptr }, depth(inDepth), voxelsPerAxis(inVoxelsPerAxis), treeActor(inTreeActor),
    parent(inParent), isLeaf(true), isVisible(false), isRoot(bInIsRoot), bounds(inBounds), regularCell(RegularCell(bufferSize))
{
    vertexFactory = MakeShareable(new FVoxelVertexFactory(bufferSize));
    regularCell = RegularCell(bufferSize);

    uint32 vertexBufferSize = voxelsPerAxis * voxelsPerAxis * voxelsPerAxis * 15;
    uint32 transitionCellBufferSize = (2 * voxelsPerAxis + 1) * (2 * voxelsPerAxis + 1);
    // vertexBufferSize += transition cell vertex count

    for (int i = 0; i < 3; i++)
        transitonCells[i] = TransitionCell(transitionCellBufferSize);
    
    ENQUEUE_RENDER_COMMAND(InitVoxelResources)(
        [this, bufferSize, transitionCellBufferSize, vertexBufferSize](FRHICommandListImmediate& RHICmdList)
        {
            vertexFactory->Initialize(vertexBufferSize);
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

 void OctreeNode::AssignChildNeighboursAsRoot() {
     for (int i = 0; i < 8; ++i) {
         OctreeNode* child = children[i];
         OctreeNode* childNeighbours[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
         FVector3f offset((i & 1 ? 1 : -1), (i & 2 ? 1 : -1), (i & 4 ? 1 : -1));

         if (offset.X == 1) childNeighbours[0] = children[i ^ 1]; // sibling +X neighbor
         else childNeighbours[0] = nullptr;

         if (offset.X == -1) childNeighbours[1] = children[i ^ 1]; // sibling -X neighbor
         else childNeighbours[1] = nullptr;

         if (offset.Y == 1) childNeighbours[2] = children[i ^ 2]; // sibling +Y neighbor
         else childNeighbours[2] = nullptr;

         if (offset.Y == -1) childNeighbours[3] = children[i ^ 2]; // sibling -Y neighbor
         else childNeighbours[3] = nullptr;

         if (offset.Z == 1) childNeighbours[4] = children[i ^ 4]; // sibling +Z neighbor
         else childNeighbours[4] = nullptr;

         if (offset.Z == -1) childNeighbours[5] = children[i ^ 4]; // sibling -Z neighbor
         else childNeighbours[5] = nullptr;

         child->AssignChildNeighbours(childNeighbours);
     }
 }

 void OctreeNode::AssignChildNeighbours(OctreeNode* inNeighbours[6]) {
     for (int i = 0; i < 8; ++i) {
         OctreeNode* child = children[i];
         OctreeNode* childNeighbours[6] = {};
         FVector3f offset((i & 1 ? 1 : -1), (i & 2 ? 1 : -1), (i & 4 ? 1 : -1));

        // +X neighbor
         if (offset.X == 1) childNeighbours[0] = children[i ^ 1];
         else if (inNeighbours[0] && inNeighbours[0]->IsLeaf() == false) 
             childNeighbours[0] = inNeighbours[0]->children[i ^ 1];
        // -X neighbor
         if (offset.X == -1) childNeighbours[1] = children[i ^ 1];
         else if (inNeighbours[1] && inNeighbours[1]->IsLeaf() == false)
             childNeighbours[1] = inNeighbours[1]->children[i ^ 1];
        // +Y neighbor
         if (offset.Y == 1) childNeighbours[2] = children[i ^ 2];
         else if (inNeighbours[2] && inNeighbours[2]->IsLeaf() == false)
             childNeighbours[2] = inNeighbours[2]->children[i ^ 2];
        // -Y neighbor
         if (offset.Y == -1) childNeighbours[3] = children[i ^ 2];
         else if (inNeighbours[3] && inNeighbours[3]->IsLeaf() == false)
             childNeighbours[3] = inNeighbours[3]->children[i ^ 2];
        // +Z neighbor
         if (offset.Z == 1) childNeighbours[4] = children[i ^ 4];
         else if (inNeighbours[4] && inNeighbours[4]->IsLeaf() == false)
             childNeighbours[4] = inNeighbours[4]->children[i ^ 4];
        // -Z neighbor
         if (offset.Z == -1) childNeighbours[5] = children[i ^ 4];
         else if (inNeighbours[5] && inNeighbours[5]->IsLeaf() == false) 
             childNeighbours[5] = inNeighbours[5]->children[i ^ 4];

         child->AssignChildNeighbours(childNeighbours);
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
        children[i] = new OctreeNode(treeActor, this, childAABB, bufferSize, voxelsPerAxis, nextDepth, maxDepth);
    }
}