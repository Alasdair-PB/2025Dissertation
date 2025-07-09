#include "OctreeNode.h"
#include "OctreeModule.h"

OctreeNode::~OctreeNode() {
	Release();
	for (int i = 0; i < 8; ++i)
		delete children[i];
}

void OctreeNode::Release() {
	avgIsoBuffer->ReleaseResource();
	avgTypeBuffer->ReleaseResource();
}