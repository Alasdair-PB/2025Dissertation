#pragma once
#include "CoreMinimal.h"
#include "OctreeNode.h"
#include "AABB.h"

class Octree {
public:
    OctreeNode* root;
    int maxDepth;

    Octree(const AABB& bounds, int depth) : maxDepth(depth) {
        root = new OctreeNode(bounds);
    }

    ~Octree() {
        delete root;
    }

    void Build(std::function<float(FVector)> sdf) {
        SubdivideRecursive(root, 0, sdf);
    }

private:
    void SubdivideRecursive(OctreeNode* node, int depth, std::function<float(FVector)> sdf) {
        if (depth >= maxDepth) return;

        if (node->CheckSubdivide(sdf)) {
            node->Subdivide();
            for (int i = 0; i < 8; ++i)
                SubdivideRecursive(node->children[i], depth + 1, sdf);
        }
    }
};
