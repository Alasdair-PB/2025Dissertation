#pragma once
#include "CoreMinimal.h"
#include "OctreeNode.h"
#include "AABB.h"
#include "VoxelRenderBuffers.h"

class OCTREE_API Octree {
public:

    Octree(float isoLevel, float scale, int inVoxelsPerAxis, int inDepth, int inBufferSizePerAxis, const TArray<float>& isoBuffer, const TArray<uint32>& typeBuffer);
    ~Octree();

    void Release();
    OctreeNode* GetRoot() { return root; }
    FBoxSphereBounds CalcVoxelBounds(const FTransform& LocalToWorld);
    TSharedPtr<FIsoUniformBuffer> GetIsoBuffer() { return isoUniformBuffer; }
    TSharedPtr<FTypeUniformBuffer> GetTypeBuffer() { return typeUniformBuffer; }

    int GetVoxelsPerAxs() const { return voxelsPerAxis; }
    int GetVoxelsPerAxsMaxRes() const { return voxelsPerAxisMaxRes; }

    float GetScale() const { return scale; }
    float GetIsoLevel() const { return isoLevel; }

    FVector3f GetOctreePosition() const {
        if (root) return root->GetBounds().Center();
        else return (FVector3f());
    }

protected:    
    OctreeNode* root;
    int maxDepth;
    TSharedPtr<FTypeUniformBuffer> typeUniformBuffer;
    TSharedPtr<FIsoUniformBuffer> isoUniformBuffer;
private:
    float scale;
    float isoLevel;
    int voxelsPerAxisMaxRes;
    int voxelsPerAxis;
    int isoValuesPerAxis = voxelsPerAxis + 1;
    int nodeVoxelCount = voxelsPerAxis * voxelsPerAxis * voxelsPerAxis;
    int isoCount = isoValuesPerAxis * isoValuesPerAxis * isoValuesPerAxis;

    /*
    bool BuildFromBuffers(const TArray<float>& isovalueBuffer, const TArray<uint32>& typeBuffer, int nodesPerAxis, int depth) {
        if (typeBuffer.Num() != (nodesPerAxis + 1) * (nodesPerAxis + 1) * (nodesPerAxis + 1)) return false;
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

    int GetLeafCount() {
        return GetLeafCountRecursive(root);
    }

private:


    struct NodeData {
        TArray<float> isoValues;
        TArray<uint32> typeValues;
    };

    int GetLeafCountRecursive(OctreeNode* node) {
        if (!node) return 0;
        if (node->IsLeaf()) return 1;
        int count = 0;
        for (int i = 0; i < 8; ++i)
            count += GetLeafCountRecursive(node->children[i]);
        return count;
    }

    TArray<NodeData> SplitIsoBufferWithOverlap(const TArray<float>& isoBuffer, const TArray<uint32>& typeBuffer, int nodesPerAxs) {
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
                    chunk.isoValues.SetNumUninitialized(chunkNodeCount * chunkNodeCount * chunkNodeCount);
                    chunk.typeValues.SetNumUninitialized(chunkNodeCount * chunkNodeCount * chunkNodeCount);


                    for (int z = 0; z < chunkNodeCount; z++) {
                        for (int y = 0; y < chunkNodeCount; y++) {
                            for (int x = 0; x < chunkNodeCount; x++) {
                                int globalX = startX + x;
                                int globalY = startY + y;
                                int globalZ = startZ + z;

                                int localIdx = (z * chunkNodeCount * chunkNodeCount) + (y * chunkNodeCount) + x;
                                chunk.isoValues[localIdx] = isoBuffer[getIndex(globalX, globalY, globalZ)];
                                chunk.typeValues[localIdx] = typeBuffer[getIndex(globalX, globalY, globalZ)];

                            }
                        }
                    }
                }
            }
        }
        return chunks;
    }

    inline bool IsHomogeneousType(const TArray<uint32>& typeBuffer, uint32 localVoxelsPerAxis) {
        uint32 firstType = typeBuffer[0];
        for (uint32 i = 1; i < (localVoxelsPerAxis + 1) * (localVoxelsPerAxis + 1) * (localVoxelsPerAxis + 1); ++i) {
            if (typeBuffer[i] != firstType)
                return false;
        }
        return true;
    }

    bool SubdivideFromBuffers(OctreeNode* node, int depth, const TArray<float> isoBuffer, const TArray<uint32> typeBuffer, uint32 localVoxelsPerAxis) {
        if (depth >= maxDepth || IsHomogeneousType(typeBuffer, localVoxelsPerAxis))
            return node->SampleValuesFromBuffers(isoBuffer, typeBuffer, localVoxelsPerAxis);

        node->Subdivide();
        if (localVoxelsPerAxis % 2 != 0) return false;

        int isoAxisSize = localVoxelsPerAxis / 2;
        TArray<NodeData> isoChunks = SplitIsoBufferWithOverlap(isoBuffer, typeBuffer, (localVoxelsPerAxis + 1));

        for (int i = 0; i < 8; ++i) {
            TArray<float>& newIsoBuffer = isoChunks[i].isoValues;
            TArray<uint32> newTypeBuffer = isoChunks[i].typeValues;
            SubdivideFromBuffers(node->children[i], depth + 1, newIsoBuffer, newTypeBuffer, isoAxisSize);
        }

        return true;
    }*/
};
