#pragma once
#include "CoreMinimal.h"
#include "AABB.h"

class OctreeNode {
public:
    AABB bounds;
    bool isLeaf;
    OctreeNode* children[8];

    OctreeNode(const AABB& b) : bounds(b), isLeaf(true) {
        for (int i = 0; i < 8; ++i)
            children[i] = nullptr;
    }

    ~OctreeNode() {
        for (int i = 0; i < 8; ++i)
            delete children[i];
    }

    void Subdivide() {
        if (!isLeaf) return;
        isLeaf = false;

        FVector c = bounds.Center();
        FVector e = bounds.Extent() * 0.5f;

        for (int i = 0; i < 8; ++i) {
            FVector offset(
                (i & 1 ? 1 : -1) * e.X,
                (i & 2 ? 1 : -1) * e.Y,
                (i & 4 ? 1 : -1) * e.Z
            );

            FVector childCenter = c + offset;
            FVector halfSize = e;

            AABB childAABB = {
                childCenter - halfSize,
                childCenter + halfSize
            };

            children[i] = new OctreeNode(childAABB);
        }
    }

    bool CheckSubdivide(std::function<float(FVector)> sdf, float threshold = 0.0f) {
        int signs = 0;
        for (int i = 0; i < 8; ++i) {
            FVector corner(
                (i & 1) ? bounds.max.X : bounds.min.X,
                (i & 2) ? bounds.max.Y : bounds.min.Y,
                (i & 4) ? bounds.max.Z : bounds.min.Z
            );
            signs |= (sdf(corner) > threshold) ? 1 : 2;
        }
        return signs == 3;
    }
};
