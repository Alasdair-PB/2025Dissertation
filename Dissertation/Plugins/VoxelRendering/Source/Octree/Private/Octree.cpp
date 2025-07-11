#include "Octree.h"
#include "OctreeModule.h"

Octree::Octree(float inIsoLevel, float inScale, int inVoxelsPerAxis, int inDepth, int inBufferSizePerAxis, const TArray<float>& isoBuffer, const TArray<uint32>& typeBuffer) :
    maxDepth(inDepth), voxelsPerAxis(inVoxelsPerAxis), scale(inScale), voxelsPerAxisMaxRes(inBufferSizePerAxis), isoLevel(inIsoLevel) {

    float scaleHalfed = inScale / 2;
    AABB bounds = { FVector3f(-scaleHalfed), FVector3f(scaleHalfed) };

    root = new OctreeNode(bounds, isoCount, 0, inDepth);
    int bufferSize = (inBufferSizePerAxis + 1) * (inBufferSizePerAxis + 1) * (inBufferSizePerAxis + 1);

    isoUniformBuffer = MakeShareable(new FIsoUniformBuffer(bufferSize));
    typeUniformBuffer = MakeShareable(new FTypeUniformBuffer(bufferSize));

    ENQUEUE_RENDER_COMMAND(InitVoxelResources)(
        [this, bufferSize, isoBuffer, typeBuffer](FRHICommandListImmediate& RHICmdList)
        {
            isoUniformBuffer->Initialize(isoBuffer, bufferSize);
            typeUniformBuffer->Initialize(typeBuffer, bufferSize);
        });
}

Octree::~Octree() {
	Release();
    FlushRenderingCommands();
    isoUniformBuffer.Reset();
    typeUniformBuffer.Reset();

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

    if (typeUniformBuffer.IsValid()) {
        ENQUEUE_RENDER_COMMAND(ReleaseTypeBufferCmd)(
            [typeUniformBuffer = typeUniformBuffer](FRHICommandListImmediate& RHICmdList) {
                typeUniformBuffer->ReleaseResource();
            });
    }
}

FBoxSphereBounds Octree::CalcVoxelBounds(const FTransform& LocalToWorld) {
    // Replace with actual bounds
    return FBoxSphereBounds(FBox(FVector(-200), FVector(200)));
}