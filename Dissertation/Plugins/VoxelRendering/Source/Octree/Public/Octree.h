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

    void BuildFromBuffers(const TArray<float>& isovalueBuffer, const TArray<uint8>& typeBuffer, int sx, int sy, int sz) {
        SubdivideFromBuffers(root, 0, isovalueBuffer, typeBuffer, sx, sy, sz);
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

    void SubdivideFromBuffers(OctreeNode* node, int depth, const TArray<float>& isovalueBuffer, const TArray<uint8>& typeBuffer, int sx, int sy, int sz) {
        node->SampleValuesFromBuffers(isovalueBuffer, typeBuffer, sx, sy, sz);
        if (depth >= maxDepth || node->IsHomogeneousType())
            return;

        node->Subdivide();
        for (int i = 0; i < 8; ++i) {
            SubdivideFromBuffers(node->children[i], depth + 1, isovalueBuffer, typeBuffer, sx, sy, sz);
        }
    }
};
