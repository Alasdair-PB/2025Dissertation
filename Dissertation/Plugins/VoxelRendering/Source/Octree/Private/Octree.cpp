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

void Octree::CheckIsoValuesDirty() {
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
    FBoxSphereBounds sphereBounds = GetBoxSphereBoundsBounds();
    //if (sphereBounds.GetBox().IsInsideOrOnXY(inPosition))
    //    return;

    //UE_LOG(LogTemp, Warning, TEXT("inPosition: %s"), *inPosition.ToString());

    float ratio = scale / 2.0;
    FVector nodeCenter = FVector(GetOctreePosition().X, GetOctreePosition().Y, GetOctreePosition().Z);
    FVector minCorner = nodeCenter - FVector(ratio, ratio, ratio) + worldPosition;

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

                if (innitIsoValue != modifiedIsoValue) bIsoValuesDirty = true;
            }
        }
    }
}
