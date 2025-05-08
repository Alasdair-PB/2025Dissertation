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

    bool BuildFromBuffers(const TArray<float>& isovalueBuffer, const TArray<uint8>& typeBuffer, int sizeX, int sizeY, int sizeZ, int depth) {
        if (typeBuffer.Num() != sizeX * sizeY * sizeZ) return false;
        if (isovalueBuffer.Num() != (sizeX + 1) * (sizeY + 1) * (sizeZ + 1)) return false;
        maxDepth = depth;
        return SubdivideFromBuffers(root, 0, isovalueBuffer, typeBuffer, 0, 0, 0, sizeX, sizeY, sizeZ);
    }

private:
    inline int TypeIndex(int x, int y, int z, int sx, int sy) {
        return x + sx * (y + sy * z);
    }

    inline int IsoIndex(int x, int y, int z, int sx, int sy) {
        return x + (sx + 1) * (y + (sy + 1) * z);
    }

    bool SubdivideFromBuffers(OctreeNode* node, int depth, const TArray<float>& isoBuffer, const TArray<uint8>& typeBuffer, 
        int startX, int startY, int startZ, int sizeX, int sizeY, int sizeZ)  {

        if (depth >= maxDepth || node->IsHomogeneousType()) {
            int typeStartIndex = TypeIndex(startX, startY, startZ, voxelsPerAxis, voxelsPerAxis);
            int isoStartIndex = IsoIndex(startX, startY, startZ, voxelsPerAxis, voxelsPerAxis);
            int typeCount = sizeX * sizeY * sizeZ;
            int allotedIsoCount = (sizeX + 1) * (sizeY + 1) * (sizeZ + 1);

            return node->SampleValuesFromBuffers(isoBuffer, typeBuffer, typeStartIndex, typeStartIndex + typeCount - 1,
                isoStartIndex, isoStartIndex + allotedIsoCount - 1);
        }

        node->Subdivide();
        int halfX = sizeX / 2, halfY = sizeY / 2, halfZ = sizeZ / 2;

        for (int i = 0; i < 8; ++i) {
            int ox = (i & 1) ? halfX : 0;
            int oy = (i & 2) ? halfY : 0;
            int oz = (i & 4) ? halfZ : 0;

            int childSizeX = (i & 1) ? (sizeX - halfX) : halfX;
            int childSizeY = (i & 2) ? (sizeY - halfY) : halfY;
            int childSizeZ = (i & 4) ? (sizeZ - halfZ) : halfZ;

            if (!SubdivideFromBuffers(node->children[i], depth + 1, isoBuffer, typeBuffer,
                startX + ox, startY + oy, startZ + oz,
                childSizeX, childSizeY, childSizeZ)){
                    UE_LOG(LogTemp, Warning, TEXT("Subdividing Octree node has failed"));
                    return false;
            }
        }
        return true;
    }
};
