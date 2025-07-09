#pragma once
#include "CoreMinimal.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FIsoFetchShaderParameters, )
    SHADER_PARAMETER_SRV(Buffer<float>, isoFetch_Buffer)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FTypeFetchShaderParameters, )
    SHADER_PARAMETER_SRV(Buffer<int>, typeFetch_Buffer)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

class OCTREE_API IIsoRenderResource : public FRenderResource {
public:
    IIsoRenderResource() : IIsoRenderResource(100) {}
    IIsoRenderResource(uint32 inCapacity) : capacity(inCapacity) {}

    virtual void Resize(const uint32& RequestedCapacity);
    virtual void ReleaseRHI() override;

    FORCEINLINE uint32 GetCapacity() const { return capacity; }

    FBufferRHIRef buffer;
    FShaderResourceViewRHIRef bufferSRV;

protected:
    uint32 capacity;
};

class OCTREE_API FIsoUniformBuffer : public IIsoRenderResource
{
public:
    FIsoUniformBuffer() : FIsoUniformBuffer(1000) {}
    FIsoUniformBuffer(uint32 inCapacity) : IIsoRenderResource(inCapacity) {}

    void Initialize(const TArray<float>& isoBuffer, int32 inCapacity);
    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
    virtual FRHIUniformBuffer* GetUniformBuffer() const { return uniformBuffer.GetReference(); }
    virtual void ReleaseRHI() override;

    TUniformBufferRef<FIsoFetchShaderParameters> uniformBuffer;
};

class FTypeUniformBuffer : public IIsoRenderResource
{
public:
    FTypeUniformBuffer() : FTypeUniformBuffer(1000) {}
    FTypeUniformBuffer(uint32 inCapacity) : IIsoRenderResource(inCapacity) {}

    void Initialize(const TArray<uint32>& typeBuffer, int32 inCapacity);
    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
    virtual FRHIUniformBuffer* GetUniformBuffer() const { return uniformBuffer.GetReference(); }
    virtual void ReleaseRHI() override;

    TUniformBufferRef<FTypeFetchShaderParameters> uniformBuffer;
};

class OCTREE_API IVoxelDynamicRenderResource : public FRenderResource {
public:
    IVoxelDynamicRenderResource() = default;
    IVoxelDynamicRenderResource(uint32 inCapacity) : capacity(inCapacity) {}

    virtual void ReleaseRHI() override;
    uint32 GetCapacity() const { return capacity; }

    FBufferRHIRef buffer;
    FShaderResourceViewRHIRef bufferSRV;
    FUnorderedAccessViewRHIRef bufferUAV;
protected:
    uint32 capacity = 0;
};

class OCTREE_API FIsoDynamicBuffer : public IVoxelDynamicRenderResource
{
public:
    FIsoDynamicBuffer() = default;
    FIsoDynamicBuffer(uint32 inCapacity) : IVoxelDynamicRenderResource(inCapacity) {}

    void Initialize(int32 capacity);
    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
};

class OCTREE_API FTypeDynamicBuffer : public IVoxelDynamicRenderResource
{
public:
    FTypeDynamicBuffer() = default;
    FTypeDynamicBuffer(uint32 inCapacity) : IVoxelDynamicRenderResource(inCapacity) {}

    void Initialize(int32 capacity);
    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
};