#include "VoxelRenderBuffers.h"
#include "OctreeModule.h"

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FIsoFetchShaderParameters, "isoFetch_Buffer");
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FTypeFetchShaderParameters, "typeFetch_Buffer");

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
    FRHIResourceCreateInfo CreateInfo(TEXT("FIsoUniformRenderBuffer"));
    EBufferUsageFlags UsageFlags = BUF_ShaderResource | BUF_Static;

    buffer = RHICmdList.CreateBuffer(sizeof(float) * capacity, UsageFlags, 0, ERHIAccess::SRVMask, CreateInfo);
    bufferSRV = RHICmdList.CreateShaderResourceView(buffer, sizeof(float), PF_R32_FLOAT);

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

void FIsoUniformBuffer::Initialize(int32 NumPoints)
{
    Resize(NumPoints);
    FRHICommandListBase& RHICmdList = FRHICommandListImmediate::Get();

    void* LockedData = RHICmdList.LockBuffer(buffer, 0, NumPoints * sizeof(float), RLM_WriteOnly);
    FMemory::Memzero(LockedData, NumPoints);
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
    FRHIResourceCreateInfo CreateInfo(TEXT("FTypeUnifromRenderBuffer"));
    EBufferUsageFlags UsageFlags = BUF_ShaderResource | BUF_Static;

    buffer = RHICmdList.CreateBuffer(sizeof(uint32) * capacity, UsageFlags, 0, ERHIAccess::SRVMask, CreateInfo);
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

void FIsoDynamicBuffer::Initialize(int32 inCapacity) {
    capacity = inCapacity;
    Resize(capacity);
    FRHICommandListBase& RHICmdList = FRHICommandListImmediate::Get();

    void* LockedData = RHICmdList.LockBuffer(buffer, 0, capacity * sizeof(float), RLM_WriteOnly);
    FMemory::Memzero(LockedData, capacity * sizeof(float));
    RHICmdList.UnlockBuffer(buffer);
}

void FIsoDynamicBuffer::InitRHI(FRHICommandListBase& RHICmdList) {
    check(capacity);
    int size = sizeof(float) * capacity;

    FRHIResourceCreateInfo CreateInfo(TEXT("FIsoDynamicRenderBuffer"));
    EBufferUsageFlags UsageFlags = BUF_ShaderResource | BUF_UnorderedAccess;
    buffer = RHICmdList.CreateBuffer(sizeof(float) * capacity, UsageFlags, 0, ERHIAccess::UAVCompute, CreateInfo);
    bufferSRV = RHICmdList.CreateShaderResourceView(buffer, sizeof(float), PF_R32_FLOAT);
    bufferUAV = RHICmdList.CreateUnorderedAccessView(buffer, PF_R32_FLOAT);

    void* LockedData = RHICmdList.LockBuffer(buffer, 0, size, RLM_WriteOnly);
    FMemory::Memzero(LockedData, size);
    RHICmdList.UnlockBuffer(buffer);
}

/**
 * Type Dynamic Buffer
 */

void FTypeDynamicBuffer::Initialize(int32 inCapacity) {
    capacity = inCapacity;
    Resize(capacity);
    FRHICommandListBase& RHICmdList = FRHICommandListImmediate::Get();

    void* LockedData = RHICmdList.LockBuffer(buffer, 0, capacity * sizeof(uint32), RLM_WriteOnly);
    FMemory::Memzero(LockedData, capacity);
    RHICmdList.UnlockBuffer(buffer);
}

void FTypeDynamicBuffer::InitRHI(FRHICommandListBase& RHICmdList) {
    check(capacity);
    int size = sizeof(uint32) * capacity;

    FRHIResourceCreateInfo CreateInfo(TEXT("FIsoDynamicRenderBuffer"));
    EBufferUsageFlags UsageFlags = BUF_ShaderResource | BUF_UnorderedAccess;
    buffer = RHICmdList.CreateBuffer(sizeof(uint32) * capacity, UsageFlags, 0, ERHIAccess::UAVCompute, CreateInfo);
    bufferSRV = RHICmdList.CreateShaderResourceView(buffer, sizeof(uint32), PF_R32_UINT);
    bufferUAV = RHICmdList.CreateUnorderedAccessView(buffer, PF_R32_UINT);

    void* LockedData = RHICmdList.LockBuffer(buffer, 0, size, RLM_WriteOnly);
    FMemory::Memzero(LockedData, size);
    RHICmdList.UnlockBuffer(buffer);
}
