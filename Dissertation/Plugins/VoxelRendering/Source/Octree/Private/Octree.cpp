#include "Octree.h"
#include "OctreeModule.h"

Octree::Octree(float inIsoLevel, float inScale, int inVoxelsPerAxis, int inDepth, int inBufferSizePerAxis, const TArray<float>& isoBuffer, const TArray<uint32>& typeBuffer, FVector inPosition) :
    maxDepth(inDepth), voxelsPerAxis(inVoxelsPerAxis), scale(inScale), voxelsPerAxisMaxRes(inBufferSizePerAxis), isoLevel(inIsoLevel), worldPosition(inPosition), bIsoValuesDirty(false) {

    float scaleHalfed = inScale / 2;
    AABB bounds = { FVector3f(-scaleHalfed), FVector3f(scaleHalfed) };

    root = new OctreeNode(bounds, isoCount, 0, inDepth);
    int bufferSize = (inBufferSizePerAxis + 1) * (inBufferSizePerAxis + 1) * (inBufferSizePerAxis + 1);

    isoUniformBuffer = MakeShareable(new FIsoUniformBuffer(bufferSize));
    deltaIsoBuffer = MakeShareable(new FIsoDynamicBuffer(bufferSize));
    typeUniformBuffer = MakeShareable(new FTypeUniformBuffer(bufferSize));
    deltaTypeBuffer = MakeShareable(new FTypeDynamicBuffer(bufferSize));

    deltaIsoArray.Init(0.0f, bufferSize);
    deltaTypeArray.Init(0, bufferSize);

    initIsoArray.SetNum(bufferSize);
    FMemory::Memcpy(initIsoArray.GetData(), isoBuffer.GetData(), bufferSize * sizeof(float));

    ENQUEUE_RENDER_COMMAND(InitVoxelResources)(
        [this, bufferSize, isoBuffer, typeBuffer](FRHICommandListImmediate& RHICmdList)
        {
            isoUniformBuffer->Initialize(isoBuffer, bufferSize);
            deltaIsoBuffer->Initialize(bufferSize);
            typeUniformBuffer->Initialize(typeBuffer, bufferSize);
            deltaTypeBuffer->Initialize(bufferSize);
        });
}

Octree::~Octree() {
	Release();
    FlushRenderingCommands();
    isoUniformBuffer.Reset();
    typeUniformBuffer.Reset();
    deltaIsoBuffer.Reset();
    deltaTypeBuffer.Reset();
    deltaIsoArray.Reset();
    deltaTypeArray.Reset();
    initIsoArray.Reset();

    // Not deleting roots due to Fatal error: A FRenderResource was deleted without being released first! To be investigated when time permits
    //delete root;
}

void Octree::Release() {
    if (isoUniformBuffer.IsValid()) {
        ENQUEUE_RENDER_COMMAND(ReleaseIsoBufferCmd)(
            [isoUniformBuffer = isoUniformBuffer](FRHICommandListImmediate& RHICmdList) {
                isoUniformBuffer->ReleaseResource();
            });
    }
    if (deltaIsoBuffer.IsValid()) {
        ENQUEUE_RENDER_COMMAND(ReleaseIsoBufferCmd)(
            [deltaIsoBuffer = deltaIsoBuffer](FRHICommandListImmediate& RHICmdList) {
                deltaIsoBuffer->ReleaseResource();
            });
    }
    if (typeUniformBuffer.IsValid()) {
        ENQUEUE_RENDER_COMMAND(ReleaseTypeBufferCmd)(
            [typeUniformBuffer = typeUniformBuffer](FRHICommandListImmediate& RHICmdList) {
                typeUniformBuffer->ReleaseResource();
            });
    }
    if (deltaTypeBuffer.IsValid()) {
        ENQUEUE_RENDER_COMMAND(ReleaseTypeBufferCmd)(
            [deltaTypeBuffer = deltaTypeBuffer](FRHICommandListImmediate& RHICmdList) {
                deltaTypeBuffer->ReleaseResource();
            });
    }
}

FBoxSphereBounds Octree::GetBoxSphereBoundsBounds() {
    // Replace with actual bounds
    // Recalculate bounds after octree change
    return FBoxSphereBounds(FBox(FVector(-200), FVector(200)));
}

FBoxSphereBounds Octree::CalcVoxelBounds(const FTransform& LocalToWorld) {
    return GetBoxSphereBoundsBounds();
}

float Octree::GetIsoSafe(const FIntVector idx)  {
    if (idx.X < 0 || idx.Y < 0 || idx.Z < 0 || idx.X >= isoValuesPerAxisMaxRes || idx.Y >= isoValuesPerAxisMaxRes || idx.Z >= isoValuesPerAxisMaxRes)
        return 1.0f;
    int flatIndex = GetIsoValueFromIndex(idx, isoValuesPerAxisMaxRes);
    return FMath::Clamp(initIsoArray[flatIndex] + deltaIsoArray[flatIndex], 0.0f, 1.0f);
 }

void Octree::GetIsoPlaneInDirection(FVector direction, FVector position,
    float& isoA, float& isoB, float& isoC, float& isoD,
    FVector& posA, FVector& posB, FVector& posC, FVector& posD) {
    FVector absDir = direction.GetAbs();
    FIntVector base = FIntVector(
        FMath::FloorToInt(position.X),
        FMath::FloorToInt(position.Y),
        FMath::FloorToInt(position.Z)
    );

    FIntVector axis1, axis2;
    if (absDir.X > absDir.Y && absDir.X > absDir.Z) {
        axis1 = FIntVector(0, 1, 0);
        axis2 = FIntVector(0, 0, 1);
    }
    else if (absDir.Y > absDir.Z) {
        axis1 = FIntVector(1, 0, 0);
        axis2 = FIntVector(0, 0, 1);
    }
    else {
        axis1 = FIntVector(1, 0, 0);
        axis2 = FIntVector(0, 1, 0);
    }

    FIntVector cornerA = base;
    FIntVector cornerB = base + axis1;
    FIntVector cornerC = base + axis2;
    FIntVector cornerD = base + axis1 + axis2;

    posA = FVector(position);
    posB = posA + FVector(axis1);
    posC = posA + FVector(axis2);
    posD = posA + FVector(axis1) + FVector(axis2);

    isoA = GetIsoSafe(cornerA);
    isoB = GetIsoSafe(cornerB);
    isoC = GetIsoSafe(cornerC);
    isoD = GetIsoSafe(cornerD);
}

bool Octree::RaycastToVoxelBody(FHitResult& hit, FVector& start, FVector& end)
{
    float ratio = scale / 2.0;
    FVector nodeCenter = FVector(GetOctreePosition().X, GetOctreePosition().Y, GetOctreePosition().Z);
    FVector extent = FVector(ratio, ratio, ratio);

    FBox bounds = FBox();
    bounds = bounds.BuildAABB(nodeCenter + worldPosition, extent*2);

    FVector direction = (end - start).GetSafeNormal();
    FVector oneOverDirection(
        FMath::IsNearlyZero(direction.X) ? 1e10f : 1.0f / direction.X,
        FMath::IsNearlyZero(direction.Y) ? 1e10f : 1.0f / direction.Y,
        FMath::IsNearlyZero(direction.Z) ? 1e10f : 1.0f / direction.Z
    );
    bool bIntersects = FMath::LineBoxIntersection(bounds, start, end, direction, oneOverDirection);
    bool bInsideOrOn = bounds.IsInside(start) || bounds.IsInside(end);

    if (!bIntersects && !bInsideOrOn) return false;
    float isoScale = scale / isoValuesPerAxisMaxRes;
    FVector minCorner = nodeCenter - extent + worldPosition;
    FVector voxelPosition = (start - minCorner) / isoScale;
    FVector endPosition = (end - minCorner) / isoScale;

    if (direction.IsZero()) return false;

    while (FVector::Distance(voxelPosition, endPosition) > 0.0) {

        if (FVector::DotProduct((endPosition - voxelPosition), direction) <= 0) break;

        float isoA, isoB, isoC, isoD;
        FVector posA, posB, posC, posD;
       
        GetIsoPlaneInDirection(direction, voxelPosition, isoA, isoB, isoC, isoD, posA, posB, posC, posD);

        if (isoA > isoLevel && isoB > isoLevel && isoC > isoLevel && isoD > isoLevel)
        {
            voxelPosition += direction;
            continue;
        }
        else if (isoA < isoLevel && isoB < isoLevel && isoC < isoLevel && isoD < isoLevel) {
            hit.Location = (voxelPosition * isoScale) + minCorner;
            return true;
        }

        FVector incrementedPosition = voxelPosition + direction;
        float isoE, isoF, isoG, isoH;
        FVector posE, posF, posG, posH;

        GetIsoPlaneInDirection(direction, incrementedPosition, isoE, isoF, isoG, isoH, posE, posF, posG, posH);

        // Do Marching cubes to ceate vertex data ofr isoA-H
        const int configIndex =
            ((isoA < isoLevel) ? 1 << 0 : 0) |
            ((isoB < isoLevel) ? 1 << 1 : 0) |
            ((isoC < isoLevel) ? 1 << 2 : 0) |
            ((isoD < isoLevel) ? 1 << 3 : 0) |
            ((isoE < isoLevel) ? 1 << 4 : 0) |
            ((isoF < isoLevel) ? 1 << 5 : 0) |
            ((isoG < isoLevel) ? 1 << 6 : 0) |
            ((isoH < isoLevel) ? 1 << 7 : 0);

        int numIndices = lengths[configIndex];
        int edgeOffset = offsets[configIndex];

        const FVector pos[8] = { posA, posB, posC, posD, posE, posF, posG, posH };
        const float iso[8] = { isoA, isoB, isoC, isoD, isoE, isoF, isoG, isoH };

        for (int i = 0; i < numIndices; i += 3)
        {
            FVector vertices[3];
            
            for (int j = 0; j < 3; ++j)
            {
                int edgeIndex = marchLookUp[edgeOffset + i + j];
                int idxA = cornerIndexAFromEdge[edgeIndex];
                int idxB = cornerIndexBFromEdge[edgeIndex];

                float valA = iso[idxA];
                float valB = iso[idxB];
                float t = (valB - valA == 0) ? 0.0f : (isoLevel - valA) / (valB - valA);
                t = FMath::Clamp(t, 0.0f, 1.0f);
                FVector vert = FMath::Lerp(pos[idxA], pos[idxB], t);
                vertices[j] = (vert);
            }

            FVector startTriRay = voxelPosition - direction;
            FVector endTriRay = incrementedPosition + direction;
            FVector outIntersectPoint;
            FVector outTriNormal = -direction;
            bool bTriRayIntersects = FMath::SegmentTriangleIntersection(startTriRay, endTriRay, vertices[0], vertices[1], vertices[2], outIntersectPoint, outTriNormal);
            if (bTriRayIntersects) {
                hit.Location = (voxelPosition * isoScale) + minCorner;
                return true;
            }
        }
        voxelPosition += direction;
    }
    return false;
}

void Octree::UpdateIsoValuesDirty() {
    if (!bIsoValuesDirty) return;

    if (deltaIsoBuffer.IsValid()) {
        float* isoPtr = deltaIsoArray.GetData();
        int bufferSize = deltaIsoBuffer->GetCapacity();

        ENQUEUE_RENDER_COMMAND(CopyIsoDelta)(
            [deltaIsoBuffer = deltaIsoBuffer, isoPtr, bufferSize](FRHICommandListImmediate& RHICmdList)
            {
                void* LockedData = RHICmdList.LockBuffer(deltaIsoBuffer->buffer, 0, bufferSize * sizeof(float), RLM_WriteOnly);
                FMemory::Memcpy(LockedData, isoPtr, bufferSize * sizeof(float));
                RHICmdList.UnlockBuffer(deltaIsoBuffer->buffer);
            });
    }
    bIsoValuesDirty = false;
}

int Octree::GetIsoValueFromIndex(FIntVector coord, int axisSize) {

    int maxIsoCount = (axisSize) * (axisSize) * (axisSize);
    int index = coord.X + coord.Y * (axisSize)+coord.Z * (axisSize) * (axisSize);
    return FMath::Max(0, FMath::Min(index, maxIsoCount + 1));
}

void Octree::ApplyDeformationAtPosition(FVector inPosition, float radius, float influence, bool additive) {

    float ratio = scale / 2.0;
    FVector nodeCenter = FVector(GetOctreePosition().X, GetOctreePosition().Y, GetOctreePosition().Z);
    FVector extent = FVector(ratio, ratio, ratio);
    FVector minCorner = nodeCenter - extent + worldPosition;

    FBox bounds = FBox();
    bounds = bounds.BuildAABB(nodeCenter + worldPosition, extent);

    if (!bounds.IsInsideOrOnXY(inPosition))
        return;

    float isoScale = scale / isoValuesPerAxisMaxRes;
    float isoRadius = radius / isoScale;
    FVector localInPosition = (inPosition - minCorner) / isoScale;

    int zMin = FMath::FloorToInt(localInPosition.Z - isoRadius);
    int zMax = FMath::CeilToInt(localInPosition.Z + isoRadius);
    int yMin = FMath::FloorToInt(localInPosition.Y - isoRadius);
    int yMax = FMath::CeilToInt(localInPosition.Y + isoRadius);
    int xMin = FMath::FloorToInt(localInPosition.X - isoRadius);
    int xMax = FMath::CeilToInt(localInPosition.X + isoRadius);

    for (int dz = zMin; dz <= zMax; dz++) {
        if (dz < 0 || dz >= isoValuesPerAxisMaxRes) continue;
        for (int dy = yMin; dy <= yMax; dy++) {
            if (dy < 0 || dy >= isoValuesPerAxisMaxRes) continue;
            for (int dx = xMin; dx <= xMax; dx++) {
                if (dx < 0 || dx >= isoValuesPerAxisMaxRes) continue;

                int flatIndex = GetIsoValueFromIndex(FIntVector3(dx, dy, dz), isoValuesPerAxisMaxRes);
                float innitIsoValue = deltaIsoArray[flatIndex];
                float modifiedIsoValue = innitIsoValue;

                float distance = FVector::Distance(FVector(dx, dy, dz), FVector(localInPosition.X, localInPosition.Y, localInPosition.Z));
                float t = FMath::Clamp(1.0f - (distance / isoRadius), 0.0f, 1.0f);
                float weightedInfluence = influence * t * t;

                additive ? modifiedIsoValue += weightedInfluence : modifiedIsoValue -= weightedInfluence;
                modifiedIsoValue = FMath::Clamp(modifiedIsoValue, 0.0f, 1.0f);

                deltaIsoArray[flatIndex] = modifiedIsoValue;
                //deltaTypeArray[flatIndex] = 3;
                if (innitIsoValue != modifiedIsoValue) bIsoValuesDirty = true;
            }
        }
    }
}
