#pragma once
#include "CoreMinimal.h"
#include "AABB.h"

static const int voxelsPerAxis = 3; 
static const int nodeVoxelCount = voxelsPerAxis * voxelsPerAxis * voxelsPerAxis;
static const int isoCount = nodeVoxelCount * 8;

class OctreeNode {
public:
    AABB bounds;
    bool isLeaf;
    OctreeNode* children[8];

    float isoValues[isoCount];
    float typeValues[nodeVoxelCount];

    OctreeNode(const AABB& b) : bounds(b), isLeaf(true) {
        for (int i = 0; i < 8; ++i)
            children[i] = nullptr;

        for (int i = 0; i < isoCount; ++i)
            isoValues[i] = 1.0f;
        FMemory::Memzero(typeValues, sizeof(float) * voxelsPerAxis * voxelsPerAxis * voxelsPerAxis);
    }

    ~OctreeNode() {
        for (int i = 0; i < 8; ++i)
            delete children[i];
    }

    bool IsHomogeneousType() const {
        uint8 firstType = typeValues[0];
        for (int i = 1; i < voxelsPerAxis * voxelsPerAxis * voxelsPerAxis; ++i) {
            if (typeValues[i] != firstType) {
                return false;
            }
        }
        return true;
    }

    void Subdivide() {
        if (!isLeaf) return;

        isLeaf = false;
        FVector3f c = bounds.Center();
        FVector3f e = bounds.Extent() * 0.5f;

        for (int i = 0; i < 8; ++i) {
            FVector3f offset((i & 1 ? 1 : -1) * e.X,(i & 2 ? 1 : -1) * e.Y,(i & 4 ? 1 : -1) * e.Z);
            FVector3f childCenter = c + offset;
            FVector3f halfSize = e;

            AABB childAABB = {childCenter - halfSize,childCenter + halfSize};
            children[i] = new OctreeNode(childAABB);
        }
    }

    bool CheckSubdivide(std::function<float(FVector3f)> sdf, float threshold = 0.0f) {
        int signs = 0;
        for (int i = 0; i < 8; ++i) {
            FVector3f corner(
                (i & 1) ? bounds.max.X : bounds.min.X,
                (i & 2) ? bounds.max.Y : bounds.min.Y,
                (i & 4) ? bounds.max.Z : bounds.min.Z
            );
            signs |= (sdf(corner) > threshold) ? 1 : 2;
        }
        return signs == 3;
    }

    int GetBufferIndex(const FVector3f& pos, int sx, int sy, int sz) {
        int voxelX = FMath::Clamp(FMath::FloorToInt(pos.X), 0, sx - 1);
        int voxelY = FMath::Clamp(FMath::FloorToInt(pos.Y), 0, sy - 1);
        int voxelZ = FMath::Clamp(FMath::FloorToInt(pos.Z), 0, sz - 1);
        return voxelZ * (sx * sy) + voxelY * sx + voxelX;
    }

    void SampleValuesFromBuffers(const TArray<float>& isovalueBuffer, const TArray<uint8>& typeBuffer, int sx, int sy, int sz) {
        FVector3f min = bounds.min;
        FVector3f max = bounds.max;
        FVector3f size = (max - min) / (voxelsPerAxis - 1);

        int index = 0;
        for (int z = 0; z < voxelsPerAxis; ++z) {
            for (int y = 0; y < voxelsPerAxis; ++y) {
                for (int x = 0; x < voxelsPerAxis; ++x) {
                    FVector3f pos = min + FVector3f(x * size.X, y * size.Y, z * size.Z);
                    isoValues[index] = isovalueBuffer[GetBufferIndex(pos, sx, sy, sz)];
                    typeValues[index] = typeBuffer[GetBufferIndex(pos, sx, sy, sz)];
                    index++;
                }
            }
        }
    }
};
