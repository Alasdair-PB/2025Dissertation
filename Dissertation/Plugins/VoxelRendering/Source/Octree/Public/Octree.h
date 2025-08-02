#pragma once
#include "CoreMinimal.h"
#include "OctreeNode.h"
#include "AABB.h"
#include "VoxelOctreeUtils.h"
#include "VoxelRenderBuffers.h"

class OCTREE_API Octree {
public:

    Octree(AActor* parent, float isoLevel, float scale, int inVoxelsPerAxis, int inDepth, int inBufferSizePerAxis,
        const TArray<float>& isoBuffer, const TArray<uint32>& typeBuffer);
    ~Octree();

    void Release();
    OctreeNode* GetRoot() { return root; }
    FBoxSphereBounds CalcVoxelBounds(const FTransform& LocalToWorld);

    TSharedPtr<FIsoUniformBuffer> GetIsoBuffer() { return isoUniformBuffer; }
    TSharedPtr<FTypeUniformBuffer> GetTypeBuffer() { return typeUniformBuffer; }
    TSharedPtr<FTypeDynamicBuffer> GetDeltaTypeBuffer() { return deltaTypeBuffer; }
    TSharedPtr<FIsoDynamicBuffer> GetDeltaIsoBuffer() { return deltaIsoBuffer; }

    TSharedPtr<FTypeDynamicBuffer> GetZeroTypeBuffer() { return zeroTypeBuffer; }
    TSharedPtr<FIsoDynamicBuffer> GetZeroIsoBuffer() { return zeroIsoBuffer; }

    TSharedPtr<FMarchingCubesLookUpResource> GetMarchLookUpResourceBuffer() { return marchingCubeLookUpTable; }

    int GetVoxelsPerAxs() const { return voxelsPerAxis; }
    int GetVoxelsPerAxsMaxRes() const { return voxelsPerAxisMaxRes; }

    float GetScale() const { return scale; }
    float GetIsoLevel() const { return isoLevel; }

    bool AreValuesDirty() const { return bIsoValuesDirty || bTypeValuesDirty; }
    bool RaycastToVoxelBody(FHitResult& hit, FVector& start, FVector& end);

    FVector3f GetOctreePosition() const {
        if (root) return root->GetBounds().Center();
        else return (FVector3f());
    }

    int GetIsoValueFromIndex(FIntVector coord, int axisSize);
    void ApplyDeformationAtPosition(FVector position, float radius, float influence, uint32 type = 0, bool additive = false);
    void UpdateIsoValuesDirty();
    void UpdateValuesDirty();
    void UpdateTypeValuesDirty();
protected:    
    FBoxSphereBounds GetBoxSphereBoundsBounds();
    OctreeNode* root;
    AActor* parent;
    int maxDepth;
    TSharedPtr<FTypeUniformBuffer> typeUniformBuffer;
    TSharedPtr<FIsoUniformBuffer> isoUniformBuffer;
    TSharedPtr<FMarchingCubesLookUpResource> marchingCubeLookUpTable;

    TSharedPtr<FIsoDynamicBuffer> deltaIsoBuffer;
    TSharedPtr<FTypeDynamicBuffer> deltaTypeBuffer;

    TSharedPtr<FIsoDynamicBuffer> zeroIsoBuffer;
    TSharedPtr<FTypeDynamicBuffer> zeroTypeBuffer;

    TArray<float> deltaIsoArray;
    TArray<float> initIsoArray;
    TArray<uint32> deltaTypeArray;
    bool bIsoValuesDirty;
    bool bTypeValuesDirty;

    float GetIsoSafe(const FIntVector position);
    void GetIsoPlaneInDirection(FVector direction, FVector position,
        float& isoA, float& isoB, float& isoC, float& isoD,
        FVector& posA, FVector& posB, FVector& posC, FVector& posD);
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
