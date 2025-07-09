#include "Octree.h"
#include "OctreeModule.h"


Octree::Octree(const AABB& bounds, int nodesPerAxis, int inDepth) : maxDepth(inDepth) {
    root = new OctreeNode(bounds);
    int bufferSize = (nodesPerAxis + 1) * (nodesPerAxis + 1) * (nodesPerAxis + 1);
    isoUniformBuffer = MakeShareable(new FIsoUniformBuffer(bufferSize));
    typeUniformBuffer = MakeShareable(new FTypeUniformBuffer(bufferSize));
}

Octree::~Octree() {
    delete root;
	Release();
}

void Octree::Release() {
	isoUniformBuffer->ReleaseResource();
	typeUniformBuffer->ReleaseResource();
}