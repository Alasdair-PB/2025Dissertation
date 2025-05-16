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

    bool BuildFromBuffers(const TArray<float>& isovalueBuffer, const TArray<uint8>& typeBuffer, int nodesPerAxis, int depth) {
        if (typeBuffer.Num() != nodesPerAxis * nodesPerAxis * nodesPerAxis) return false;
        if (isovalueBuffer.Num() != (nodesPerAxis + 1) * (nodesPerAxis + 1) * (nodesPerAxis + 1)) return false;

        maxDepth = depth;
        return SubdivideFromBuffers(root, 0, isovalueBuffer, typeBuffer, nodesPerAxis);
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
    struct NodeData {
        TArray<float> isoValues;
        FIntVector size;
    };

    TArray<NodeData> SplitIsoBufferWithOverlap(const TArray<float>& isoBuffer, int nodesPerAxs) {
        TArray<NodeData> chunks;
        chunks.SetNum(8);

        int halfNodePerAxis = nodesPerAxs / 2;
        int chunkNodeCount = halfNodePerAxis + 1; 

        auto getIndex = [&](int xi, int yi, int zi) {
            return (zi * nodesPerAxs * nodesPerAxs) + (yi * nodesPerAxs) + xi;};

        for (int cz = 0; cz < 2; ++cz) {
            for (int cy = 0; cy < 2; ++cy) {
                for (int cx = 0; cx < 2; ++cx) {
                    int chunkIndex = cz * 4 + cy * 2 + cx;

                    int startX = cx * halfNodePerAxis;
                    int startY = cy * halfNodePerAxis;
                    int startZ = cz * halfNodePerAxis;

                    NodeData& chunk = chunks[chunkIndex];
                    chunk.size = FIntVector(chunkNodeCount, chunkNodeCount, chunkNodeCount);
                    chunk.isoValues.SetNumUninitialized(chunkNodeCount * chunkNodeCount * chunkNodeCount);

                    for (int z = 0; z < chunkNodeCount; z++) {
                        for (int y = 0; y < chunkNodeCount; y++) {
                            for (int x = 0; x < chunkNodeCount; x++) {
                                int globalX = startX + x;
                                int globalY = startY + y;
                                int globalZ = startZ + z;

                                int localIdx = (z * chunkNodeCount * chunkNodeCount) + (y * chunkNodeCount) + x;
                                chunk.isoValues[localIdx] = isoBuffer[getIndex(globalX, globalY, globalZ)];
                            }
                        }
                    }
                }
            }
        }
        return chunks;
    }

    inline bool IsHomogeneousType(const TArray<uint8>& typeBuffer, uint32 localVoxelsPerAxis) {
        uint8 firstType = typeBuffer[0];
        for (uint32 i = 1; i < localVoxelsPerAxis * localVoxelsPerAxis * localVoxelsPerAxis; ++i) {
            if (typeBuffer[i] != firstType)
                return false;
        }
        return true;
    }

    bool SubdivideFromBuffers(OctreeNode* node, int depth, const TArray<float> isoBuffer, const TArray<uint8> typeBuffer, uint32 localVoxelsPerAxis) {
        if (depth >= maxDepth || IsHomogeneousType(typeBuffer, localVoxelsPerAxis))
            return node->SampleValuesFromBuffers(isoBuffer, typeBuffer, localVoxelsPerAxis);

        node->Subdivide();
        if (typeBuffer.Num() % 8 != 0) return false;
        if (localVoxelsPerAxis % 2 != 0) return false;

        int nodeSize = typeBuffer.Num() / 8;
        int isoAxisSize = localVoxelsPerAxis / 2;

        TArray<NodeData> isoChunks = SplitIsoBufferWithOverlap(isoBuffer, (localVoxelsPerAxis + 1));

        for (int i = 0; i < 8; ++i) {
            TArray<float>& newIsoBuffer = isoChunks[i].isoValues;
            TArray<uint8> newTypeBuffer;
            newTypeBuffer.Append(typeBuffer.GetData() + i * nodeSize, nodeSize);
            SubdivideFromBuffers(node->children[i], depth + 1, newIsoBuffer, newTypeBuffer, isoAxisSize);
        }

        return true;
    }
};
