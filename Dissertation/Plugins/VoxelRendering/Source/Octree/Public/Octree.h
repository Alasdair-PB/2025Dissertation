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

    static int32 IntPow(int32 base, int32 exponent) {
        int32 result = 1;
        while (exponent > 0) {
            if (exponent % 2 == 1)
                result *= base;
            base *= base;
            exponent /= 2;
        }
        return result;
    }

private:
    inline int TypeIndex(int x, int y, int z, int sx, int sy) {
        return x + sx * (y + sy * z);
    }

    inline int IsoIndex(int x, int y, int z, int strideY, int strideZ) {
        return x + strideY * (y + strideZ * z);
    }

    bool SubdivideFromBuffers(OctreeNode* node, int depth, const TArray<float>& isoBuffer, const TArray<uint8>& typeBuffer, 
        int startX, int startY, int startZ, int sizeX, int sizeY, int sizeZ)  {

        if (depth >= maxDepth || node->IsHomogeneousType()) {
            int typeStartIndex = TypeIndex(startX, startY, startZ, voxelsPerAxis, voxelsPerAxis);
            int nodesPerAxisMaxRes = IntPow(8, depth);
            int fullStrideY = (voxelsPerAxis * nodesPerAxisMaxRes) + 1;
            int fullStrideZ = (voxelsPerAxis * nodesPerAxisMaxRes) + 1;
            int isoStartIndex = IsoIndex(startX, startY, startZ, fullStrideY, fullStrideZ);

            int typeNum = sizeX * sizeY * sizeZ;
            int isoNum = (sizeX + 1) * (sizeY + 1) * (sizeZ + 1);

            return node->SampleValuesFromBuffers(
                isoBuffer, typeBuffer,
                typeStartIndex, typeStartIndex + typeNum - 1,
                isoStartIndex, isoStartIndex + isoNum - 1
            );
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
