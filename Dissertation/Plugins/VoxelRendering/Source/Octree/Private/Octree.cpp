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
    delete root;
	Release();
}

void Octree::Release() {
	isoUniformBuffer->ReleaseResource();
	typeUniformBuffer->ReleaseResource();
}

FBoxSphereBounds Octree::CalcVoxelBounds(const FTransform& LocalToWorld) {
    // Replace with actual bounds
    return FBoxSphereBounds(FBox(FVector(-200), FVector(200)));
}