#pragma once
#include "CoreMinimal.h"
#include "AABB.h"

static const int voxelsPerAxis = 3; 
static const int nodeVoxelCount = voxelsPerAxis * voxelsPerAxis * voxelsPerAxis;
static const int isoCount = (voxelsPerAxis + 1) * (voxelsPerAxis + 1) * (voxelsPerAxis + 1);

class OctreeNode {
public:
    AABB bounds;
    bool isLeaf;
    OctreeNode* children[8];

    int32 allocatedIndexStart;
    int32 allocatedIndexEnd; 

    float isoAveragedValues[isoCount];
    float typeValues[nodeVoxelCount];

    OctreeNode(const AABB& b) : bounds(b), isLeaf(true) {
        for (int i = 0; i < 8; ++i)
            children[i] = nullptr;

        for (int i = 0; i < isoCount; ++i)
            isoAveragedValues[i] = 1.0f;
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

    int GetAverageType(int i, int averagePassCount, const TArray<uint8>& typeBuffer) {
        int mostCommonIndex = 0;
        int maxCount = 0;
        TMap<int, int> frequencyMap;

        for (int j = 0; j < averagePassCount; j++) {
            int value = typeBuffer[i + j];
            frequencyMap.FindOrAdd(value)++;
        }

        for (const auto& pair : frequencyMap) {
            if (pair.Value > maxCount) {
                mostCommonIndex = pair.Key;
                maxCount = pair.Value;
            }
        }
        return mostCommonIndex;
    }


    bool SampleValuesFromBuffers(const TArray<float> isovalueBuffer, const TArray<uint8> typeBuffer, int localVoxelsPerAxis) {
        int allocatedTypeCount = localVoxelsPerAxis * localVoxelsPerAxis * localVoxelsPerAxis;
        int highResCount = (localVoxelsPerAxis + 1) * (localVoxelsPerAxis + 1) * (localVoxelsPerAxis + 1);
        int scale = localVoxelsPerAxis / voxelsPerAxis;

        if (allocatedTypeCount % nodeVoxelCount != 0) {
            UE_LOG(LogTemp, Warning, TEXT("Valid typeValues have not been provided for this level of detail"));
            return false;
        }

        if (isovalueBuffer.Num() != highResCount) {
            UE_LOG(LogTemp, Warning, TEXT("isovalueBuffer size mismatch"));
            return false;
        }

        int averagePassCount = allocatedTypeCount / nodeVoxelCount;
        for (int localIndex = 0; localIndex < nodeVoxelCount; ++localIndex) {
            int baseIndex = localIndex * averagePassCount;
            typeValues[localIndex] = GetAverageType(baseIndex, averagePassCount, typeBuffer);
        }

        for (int z = 0; z <= voxelsPerAxis; ++z) {
            for (int y = 0; y <= voxelsPerAxis; ++y) {
                for (int x = 0; x <= voxelsPerAxis; ++x) {
                    float sum = 0.0f;
                    int count = 0;

                    for (int dz = 0; dz < scale; ++dz) {
                        for (int dy = 0; dy < scale; ++dy) {
                            for (int dx = 0; dx < scale; ++dx) {
                                int hx = x * scale + dx;
                                int hy = y * scale + dy;
                                int hz = z * scale + dz;

                                if (hx < 0 || hy < 0 || hz < 0 ||
                                    hx > localVoxelsPerAxis ||
                                    hy > localVoxelsPerAxis ||
                                    hz > localVoxelsPerAxis) {
                                    continue;
                                }
                                int highIndex = hx + (voxelsPerAxis + 1) * (hy + (voxelsPerAxis + 1) * hz);

                                sum += isovalueBuffer[highIndex];
                                ++count;
                            }
                        }
                    }
                    int lowIndex = x + (voxelsPerAxis + 1) * (y + (voxelsPerAxis + 1) * z);
                    isoAveragedValues[lowIndex] = sum / count;
                }
            }
        }

        return true;
    }
};
