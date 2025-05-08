#pragma once
#include "CoreMinimal.h"
#include "OctreeNode.h"
#include "AABB.h"

class Octree {
public:
    OctreeNode* root;
    int maxDepth;

    Octree(const AABB& bounds) : maxDepth(0) {
        root = new OctreeNode(bounds);
    }

    ~Octree() {
        delete root;
    }

    bool BuildFromBuffers(const TArray<float>& isovalueBuffer, const TArray<uint8>& typeBuffer) {
        if (typeBuffer.Num() % 8 != 0 || isovalueBuffer.Num() % 8 != 0) return false;
        maxDepth = typeBuffer.Num() / 8;
        return SubdivideFromBuffers(root, 0, isovalueBuffer, typeBuffer, 0, typeBuffer.Num() - 1, 0, isovalueBuffer.Num() - 1);
    }

private:
    bool SubdivideFromBuffers(OctreeNode* node, int depth, const TArray<float>& isovalueBuffer, const TArray<uint8>& typeBuffer, int startTypeIndex, int endTypeIndex, int startIsoIndex, int endIsoIndex) {
        node->SampleValuesFromBuffers(isovalueBuffer, typeBuffer, startTypeIndex, endTypeIndex, startIsoIndex, endIsoIndex);

        if (depth >= maxDepth || node->IsHomogeneousType()) return;
        node->Subdivide();

        int totalTypeSize = endTypeIndex - startTypeIndex + 1;
        int totalIsoSize = endIsoIndex - startIsoIndex + 1;

        if (totalTypeSize % 8 != 0 || totalIsoSize % 8 != 0) return false;

        int allocatedTypeSize = totalTypeSize / 8;
        int allocatedIsoSize = totalIsoSize / 8;

        for (int i = 0; i < 8; ++i) {
            int newTypeStart = startTypeIndex + (allocatedTypeSize * i);
            int newTypeEnd = (i == 7) ? endTypeIndex : startTypeIndex + (allocatedTypeSize * (i + 1)) - 1;

            int newIsoStart = startIsoIndex + (allocatedIsoSize * i);
            int newIsoEnd = (i == 7) ? endIsoIndex : startIsoIndex + (allocatedIsoSize * (i + 1)) - 1;

            if (!SubdivideFromBuffers(node->children[i], depth + 1, isovalueBuffer, typeBuffer, newTypeStart, newTypeEnd, newIsoStart, newIsoEnd))
                return false;
        }
        return true;
    }
};
