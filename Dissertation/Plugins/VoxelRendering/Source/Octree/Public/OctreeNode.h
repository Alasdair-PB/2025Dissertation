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

    float GetAveragedIsoValue(int i, int averagePassCount, const TArray<float>& isovalueBuffer) {
        float sum = 0.0f;
        for (int j = 0; j < averagePassCount; j++)
            sum += isovalueBuffer[i + j];
        return sum / averagePassCount;
    }

    // Optimization::Could be made parallel
    bool SampleValuesFromBuffers(const TArray<float>& isovalueBuffer, const TArray<uint8>& typeBuffer, int startTypeIndex, int endTypeIndex, int startIsoIndex, int endIsoIndex) {
        int allocatedTypeCount = (endTypeIndex - startTypeIndex + 1);        
        int allocatedIsoCount = (endIsoIndex - startIsoIndex + 1);

        if (allocatedTypeCount % nodeVoxelCount != 0) return false; // Valid isoValues have not been provided for this level of detail
        if (allocatedIsoCount % isoCount != 0) return false; // Valid isoValues have not been provided for this level of detail

        int averagePassCount = allocatedTypeCount / nodeVoxelCount;

        for (int localIndex = 0; localIndex < nodeVoxelCount; ++localIndex) {
            int baseIndex = startTypeIndex + localIndex * averagePassCount;
            typeValues[localIndex] = GetAverageType(baseIndex, averagePassCount, typeBuffer);
        }

        int isoAveragePassCount = allocatedIsoCount / isoCount;
        for (int localIndex = 0; localIndex < isoCount; ++localIndex) {
            int baseIndex = startIsoIndex + localIndex * isoAveragePassCount;
            isoAveragedValues[localIndex] = GetAveragedIsoValue(baseIndex, isoAveragePassCount, isovalueBuffer);
        }

        return true;
    }
};
