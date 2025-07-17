#pragma once
#include "CoreMinimal.h"
#include "OctreeNode.h"
#include "AABB.h"
#include "VoxelRenderBuffers.h"

class OCTREE_API Octree {
public:

    Octree(float isoLevel, float scale, int inVoxelsPerAxis, int inDepth, int inBufferSizePerAxis, 
        const TArray<float>& isoBuffer, const TArray<uint32>& typeBuffer, FVector inPosition);
    ~Octree();

    void Release();
    OctreeNode* GetRoot() { return root; }
    FBoxSphereBounds CalcVoxelBounds(const FTransform& LocalToWorld);

    TSharedPtr<FIsoUniformBuffer> GetIsoBuffer() { return isoUniformBuffer; }
    TSharedPtr<FTypeUniformBuffer> GetTypeBuffer() { return typeUniformBuffer; }
    TSharedPtr<FTypeDynamicBuffer> GetDeltaTypeBuffer() { return deltaTypeBuffer; }
    TSharedPtr<FIsoDynamicBuffer> GetDeltaIsoBuffer() { return deltaIsoBuffer; }

    int GetVoxelsPerAxs() const { return voxelsPerAxis; }
    int GetVoxelsPerAxsMaxRes() const { return voxelsPerAxisMaxRes; }

    float GetScale() const { return scale; }
    float GetIsoLevel() const { return isoLevel; }

    // LocalPosition
    FVector3f GetOctreePosition() const {
        if (root) return root->GetBounds().Center();
        else return (FVector3f());
    }
    int GetIsoValueFromIndex(FIntVector coord, int axisSize);
    void ApplyDeformationAtPosition(FVector position, float radius, float influence, bool additive = true);
    void CheckIsoValuesDirty();

protected:    
    FBoxSphereBounds GetBoxSphereBoundsBounds();
    OctreeNode* root;
    int maxDepth;
    FVector worldPosition;
    TSharedPtr<FTypeUniformBuffer> typeUniformBuffer;
    TSharedPtr<FIsoUniformBuffer> isoUniformBuffer;

    TSharedPtr<FIsoDynamicBuffer> deltaIsoBuffer;
    TSharedPtr<FTypeDynamicBuffer> deltaTypeBuffer;
    TArray<float> deltaIsoArray;
    TArray<int> deltaTypeArray;
    bool bIsoValuesDirty;

private:
    float scale;
    float isoLevel;
    int voxelsPerAxisMaxRes;
    int isoValuesPerAxisMaxRes = voxelsPerAxisMaxRes + 1;
    int voxelsPerAxis;
    int isoValuesPerAxis = voxelsPerAxis + 1;
    int nodeVoxelCount = voxelsPerAxis * voxelsPerAxis * voxelsPerAxis;
    int isoCount = isoValuesPerAxis * isoValuesPerAxis * isoValuesPerAxis;
};
