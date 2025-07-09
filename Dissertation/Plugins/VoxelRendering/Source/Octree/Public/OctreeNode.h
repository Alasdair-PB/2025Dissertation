#pragma once
#include "CoreMinimal.h"
#include "AABB.h"
#include "FVoxelVertexFactory.h"
#include "RenderResource.h"
#include "VoxelRenderBuffers.h"

#include "Materials/MaterialRelevance.h"
class FVoxelVertexFactory;

static const int voxelsPerAxis = 5;
static const int isoValuesPerAxis = voxelsPerAxis + 1;
static const int nodeVoxelCount = voxelsPerAxis * voxelsPerAxis * voxelsPerAxis;
static const int isoCount = isoValuesPerAxis * isoValuesPerAxis * isoValuesPerAxis;


class OctreeNode {
public:
    TSharedPtr<FVoxelVertexFactory> GetVertexFactory(){ return vertexFactory; }

    AABB bounds;
    OctreeNode* children[8];

    float isoValues[isoCount];
    float deformedIsoValues[isoCount];

    uint32 typeValues[isoCount];
    uint32 deformedTypeValues[isoCount];

protected:
    bool isLeaf;
    int32 maxVertexIndex;

    TSharedPtr<FVoxelVertexFactory> vertexFactory;
    TSharedPtr<FIsoDynamicBuffer> avgIsoBuffer;
    TSharedPtr<FTypeDynamicBuffer> avgTypeBuffer;

public:

    OctreeNode(ERHIFeatureLevel::Type InFeatureLevel, uint32 bufferSize) {
        // 	VertexFactory = new FVoxelVertexFactory(GetScene().GetFeatureLevel(), size);
        vertexFactory = MakeShareable(new FVoxelVertexFactory(InFeatureLevel, bufferSize));
        avgIsoBuffer = MakeShareable(new FIsoDynamicBuffer(bufferSize));
        avgTypeBuffer = MakeShareable(new FTypeDynamicBuffer(bufferSize));

        vertexFactory->Initialize(bufferSize);
        avgTypeBuffer->Initialize(bufferSize);
        avgTypeBuffer->Initialize(bufferSize);
    }

    OctreeNode(const AABB& b) : bounds(b), isLeaf(true) {
        for (int i = 0; i < 8; ++i)
            children[i] = nullptr;

        for (int i = 0; i < isoCount; ++i) {
            isoValues[i] = 1.0f;
            typeValues[i] = 0;
        }
    }

    ~OctreeNode();
    void Release();

    bool IsLeaf() { return isLeaf; }

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
