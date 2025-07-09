#include "VoxelRenderBuffers.h"
#include "OctreeModule.h"

/**
 * Base Uniform Buffer
 */

void IIsoRenderResource::Resize(const uint32& requestedCapacity)
{
    FRHICommandListBase& RHICmdList = FRHICommandListImmediate::Get();

    if (capacity != requestedCapacity)
    {
        ReleaseResource();
        capacity = requestedCapacity;
        InitResource(RHICmdList);
    }
    else if (!IsInitialized())
        InitResource(RHICmdList);
}

void IIsoRenderResource::ReleaseRHI()
{
    check(IsInRenderingThread());
    if (buffer) buffer.SafeRelease();
    bufferSRV.SafeRelease();
}

/**
 * Iso Uniform Buffer
 */

void FIsoUniformBuffer::InitRHI(FRHICommandListBase& RHICmdList)
{
    FRHIResourceCreateInfo CreateInfo(TEXT("FIsoRenderBuffer"));
    buffer = RHICmdList.CreateVertexBuffer(sizeof(uint32) * capacity, BUF_ShaderResource | BUF_Dynamic, CreateInfo);
    bufferSRV = RHICmdList.CreateShaderResourceView(buffer, sizeof(uint32), PF_R32_FLOAT);

    FIsoFetchShaderParameters uniformParameters;
    uniformParameters.isoFetch_Buffer = bufferSRV;
    uniformBuffer = TUniformBufferRef<FIsoFetchShaderParameters>::CreateUniformBufferImmediate(uniformParameters, UniformBuffer_MultiFrame);
}

void FIsoUniformBuffer::Initialize(const TArray<float>& isoBuffer, int32 NumPoints)
{
    Resize(NumPoints);
    FRHICommandListBase& RHICmdList = FRHICommandListImmediate::Get();

    uint8* structuredBuffer = (uint8*)RHICmdList.LockBuffer(buffer, 0, NumPoints * sizeof(float), RLM_WriteOnly);
    FMemory::Memcpy(structuredBuffer, isoBuffer.GetData(), isoBuffer.Num() * sizeof(float));
    RHICmdList.UnlockBuffer(buffer);
}

void FIsoUniformBuffer::ReleaseRHI()
{
    check(IsInRenderingThread());
    if (uniformBuffer.IsValid()) uniformBuffer.SafeRelease();
    if (buffer) buffer.SafeRelease();

    bufferSRV.SafeRelease();
}

/**
 * Type Uniform Buffer
 */

void FTypeUniformBuffer::InitRHI(FRHICommandListBase& RHICmdList)
{
    FRHIResourceCreateInfo CreateInfo(TEXT("FIsoRenderBuffer"));
    buffer = RHICmdList.CreateVertexBuffer(sizeof(uint32) * capacity, BUF_ShaderResource | BUF_Dynamic, CreateInfo);
    bufferSRV = RHICmdList.CreateShaderResourceView(buffer, sizeof(uint32), PF_R32_UINT);

    FTypeFetchShaderParameters uniformParameters;
    uniformParameters.typeFetch_Buffer = bufferSRV;
    uniformBuffer = TUniformBufferRef<FTypeFetchShaderParameters>::CreateUniformBufferImmediate(uniformParameters, UniformBuffer_MultiFrame);
}

void FTypeUniformBuffer::Initialize(const TArray<uint32>& typeBuffer, int32 NumPoints)
{
    Resize(NumPoints);
    FRHICommandListBase& RHICmdList = FRHICommandListImmediate::Get();

    uint8* structuredBuffer = (uint8*)RHICmdList.LockBuffer(buffer, 0, NumPoints * sizeof(uint32), RLM_WriteOnly);
    FMemory::Memcpy(structuredBuffer, typeBuffer.GetData(), typeBuffer.Num() * sizeof(uint32));
    RHICmdList.UnlockBuffer(buffer);
}

void FTypeUniformBuffer::ReleaseRHI()
{
    check(IsInRenderingThread());
    if (uniformBuffer.IsValid()) uniformBuffer.SafeRelease();
    if (buffer) buffer.SafeRelease();

    bufferSRV.SafeRelease();
}

/**
 * Base Dynamic Buffer
 */

void IVoxelDynamicRenderResource::ReleaseRHI() 
{
    check(IsInRenderingThread());

    if (buffer) buffer.SafeRelease();
    bufferUAV.SafeRelease();
    bufferSRV.SafeRelease();
}

/**
 * Iso Dynamic Buffer
 */


void FIsoDynamicBuffer::Initialize(int32 capacity) {

}

void FIsoDynamicBuffer::InitRHI(FRHICommandListBase& RHICmdList) {

}

/**
 * Type Dynamic Buffer
 */

void FTypeDynamicBuffer::Initialize(int32 capacity) {

}

void FTypeDynamicBuffer::InitRHI(FRHICommandListBase& RHICmdList) {

}
