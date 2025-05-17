#pragma once
#include "CoreMinimal.h"
#include "AABB.h"

static const int voxelsPerAxis = 5;
static const int isoValuesPerAxis = voxelsPerAxis + 1;
static const int nodeVoxelCount = voxelsPerAxis * voxelsPerAxis * voxelsPerAxis;
static const int isoCount = isoValuesPerAxis * isoValuesPerAxis * isoValuesPerAxis;

struct IsoCorner {
    float isoValue; 
    int typeValue;
};

class OctreeNode {
public:
    AABB bounds;
    bool isLeaf;
    OctreeNode* children[8];

    int32 allocatedIndexStart;
    int32 allocatedIndexEnd; 
    float isoValues[isoCount];
    uint32 typeValues[isoCount];

    OctreeNode(const AABB& b) : bounds(b), isLeaf(true) {
        for (int i = 0; i < 8; ++i)
            children[i] = nullptr;

        for (int i = 0; i < isoCount; ++i) {
            isoValues[i] = 1.0f;
            typeValues[i] = 0;
        }
    }

    ~OctreeNode() {
        for (int i = 0; i < 8; ++i)
            delete children[i];
    }

    bool IsHomogeneousType() const {
        uint32 firstType = typeValues[0];
        for (int i = 1; i < isoValuesPerAxis * isoValuesPerAxis * isoValuesPerAxis; ++i) {
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


    bool SampleValuesFromBuffers(const TArray<float> isovalueBuffer, const TArray<uint32> typeBuffer, int localVoxelsPerAxis) {
        int allocatedTypeCount = localVoxelsPerAxis * localVoxelsPerAxis * localVoxelsPerAxis;
        int highResCount = (localVoxelsPerAxis + 1) * (localVoxelsPerAxis + 1) * (localVoxelsPerAxis + 1);
        int scale = localVoxelsPerAxis / voxelsPerAxis;

        if (typeBuffer.Num() != highResCount) {
            UE_LOG(LogTemp, Warning, TEXT("Valid typeValues have not been provided for this level of detail"));
            return false;
        }

        if (isovalueBuffer.Num() != highResCount) {
            UE_LOG(LogTemp, Warning, TEXT("isovalueBuffer size mismatch"));
            return false;
        }

        for (int z = 0; z <= voxelsPerAxis; ++z) {
            for (int y = 0; y <= voxelsPerAxis; ++y) {
                for (int x = 0; x <= voxelsPerAxis; ++x) {
                    float sum = 0.0f;
                    int count = 0;

                    int mostCommonTypeIndex = 0;
                    int maxTypeCount = 0;
                    TMap<int, int> frequencyMap;

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

                                int value = typeBuffer[highIndex];
                                frequencyMap.FindOrAdd(value)++;

                                sum += isovalueBuffer[highIndex];
                                ++count;
                            }
                        }
                    }

                    for (const auto& pair : frequencyMap) {
                        if (pair.Value > maxTypeCount) {
                            mostCommonTypeIndex = pair.Key;
                            maxTypeCount = pair.Value;
                        }
                    }

                    int lowIndex = x + (voxelsPerAxis + 1) * (y + (voxelsPerAxis + 1) * z);
                    isoValues[lowIndex] = sum / count;
                    typeValues[lowIndex] = mostCommonTypeIndex;

                }
            }
        }

        return true;
    }
};
