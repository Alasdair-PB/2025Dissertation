#pragma once
#include "CoreMinimal.h"
#include "AABB.h"
#include "FVoxelVertexFactory.h"
#include "RenderResource.h"
#include "VoxelRenderBuffers.h"
#include "Materials/MaterialRelevance.h"

class FVoxelVertexFactory;

class OctreeNode {
public:
    OctreeNode(const AABB& inBounds, uint32 bufferSize, int depth, int maxDepth, int inSubdivisionIndex = 0);
    ~OctreeNode();

    void Release();
    bool IsLeaf() { return isLeaf; }

    TSharedPtr<FVoxelVertexFactory> GetVertexFactory(){ return vertexFactory; }
    TSharedPtr<FIsoDynamicBuffer> GetIsoBuffer() { return avgIsoBuffer; }
    TSharedPtr<FTypeDynamicBuffer> GetTypeBuffer() { return avgTypeBuffer; }

    AABB GetBounds() const { return bounds; }
    OctreeNode* children[8];
    int GetDepth() const { return depth; }
    int GetSubdivisionIndex() const { return depth; }

protected:
    int maxVertexIndex;
    int depth;
    int subDivisonIndex;

    bool isLeaf;
    bool isVisible;
    AABB bounds;

    TSharedPtr<FVoxelVertexFactory> vertexFactory;
    TSharedPtr<FIsoDynamicBuffer> avgIsoBuffer;
    TSharedPtr<FTypeDynamicBuffer> avgTypeBuffer;

    void Subdivide(int inDepth, int maxDepth, int bufferSize);


    //float isoValues[isoCount];
    //uint32 typeValues[isoCount];

    /*
    bool IsHomogeneousType() const {
        uint32 firstType = typeValues[0];
        for (int i = 1; i < isoValuesPerAxis * isoValuesPerAxis * isoValuesPerAxis; ++i) {
            if (typeValues[i] != firstType) {
                return false;
            }
        }
        return true;
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
    }*/
};
