#pragma once
#include "CoreMinimal.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FIsoFetchShaderParameters, )
    SHADER_PARAMETER_SRV(Buffer<float>, isoFetch_Buffer)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FTypeFetchShaderParameters, )
    SHADER_PARAMETER_SRV(Buffer<int>, typeFetch_Buffer)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

class VOXELRENDERINGUTILS_API IIsoRenderResource : public FRenderResource {
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

class VOXELRENDERINGUTILS_API FMarchingCubesLookUpResource : public FRenderResource
{
public:
    FMarchingCubesLookUpResource() {}

    void Initialize();
    void InitRHI(FRHICommandListBase& RHICmdList) override;
    void ReleaseRHI() override;

    FBufferRHIRef marchLookUpBuffer;
    FBufferRHIRef transVoxelLookUpBuffer;
    FBufferRHIRef transVoxelVertexLookUpBuffer;

    FShaderResourceViewRHIRef marchLookUpBufferSRV;
    FShaderResourceViewRHIRef  transVoxelLookUpBufferSRV;
    FShaderResourceViewRHIRef transVoxelVertexLookUpBufferSRV;

};

class VOXELRENDERINGUTILS_API FIsoUniformBuffer : public IIsoRenderResource
{
public:
    FIsoUniformBuffer() : FIsoUniformBuffer(1000) {}
    FIsoUniformBuffer(uint32 inCapacity) : IIsoRenderResource(inCapacity) {}

    void Initialize(const TArray<float>& isoBuffer, int32 inCapacity);
    void Initialize(int32 inCapacity);

    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
    virtual FRHIUniformBuffer* GetUniformBuffer() const { return uniformBuffer.GetReference(); }
    virtual void ReleaseRHI() override;

    TUniformBufferRef<FIsoFetchShaderParameters> uniformBuffer;
};

class VOXELRENDERINGUTILS_API FTypeUniformBuffer : public IIsoRenderResource
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

class VOXELRENDERINGUTILS_API IVoxelDynamicRenderResource : public IIsoRenderResource {
public:
    IVoxelDynamicRenderResource() = default;
    IVoxelDynamicRenderResource(uint32 inCapacity) : IIsoRenderResource(inCapacity) {}

    virtual void ReleaseRHI() override;
    FUnorderedAccessViewRHIRef bufferUAV;
};

class VOXELRENDERINGUTILS_API FIsoDynamicBuffer : public IVoxelDynamicRenderResource
{
public:
    FIsoDynamicBuffer() = default;
    FIsoDynamicBuffer(uint32 inCapacity) : IVoxelDynamicRenderResource(inCapacity) {}

    void Initialize(int32 capacity);
    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
};

class VOXELRENDERINGUTILS_API FTypeDynamicBuffer : public IVoxelDynamicRenderResource
{
public:
    FTypeDynamicBuffer() = default;
    FTypeDynamicBuffer(uint32 inCapacity) : IVoxelDynamicRenderResource(inCapacity) {}

    void Initialize(int32 capacity);
    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
};


class VOXELRENDERINGUTILS_API FVoxelIndexBuffer : public FIndexBuffer
{
public:
    FVoxelIndexBuffer() = default;
    ~FVoxelIndexBuffer() = default;
    FVoxelIndexBuffer(uint32 InNumIndices) : numIndices(InNumIndices) {}

    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
    virtual void ReleaseRHI() override;

    uint32 GetIndexCount() const { return numIndices; }
    uint32 GetVisibleIndiceCount() const { return visibleIndicies; }
    void SetVisibleIndiciesCount(uint32 inVisibleIndicies) { visibleIndicies = inVisibleIndicies; }

    void SetElementCount(uint32 inNumIndices) {
        numIndices = inNumIndices;
        visibleIndicies = inNumIndices;
    }

    FShaderResourceViewRHIRef SRV;
private:
    uint32 numIndices = 0;
    uint32 visibleIndicies = 0;
};

class VOXELRENDERINGUTILS_API FVoxelVertexBuffer : public FVertexBuffer
{
public:
    FVoxelVertexBuffer() = default;
    ~FVoxelVertexBuffer() = default;
    FVoxelVertexBuffer(uint32 InNumVertices) : numVertices(InNumVertices) {}

    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
    virtual void ReleaseRHI() override;
    uint32 GetVertexCount() const { return numVertices; }

    void SetElementCount(uint32 InNumVertices) {
        numVertices = InNumVertices;
        visibleVerticies = InNumVertices;
    }

    uint32 GetVisibleVerticiesCount() const { return visibleVerticies; }
    void SetVisibleVerticiessCount(uint32 inVisibleVerticies) { visibleVerticies = inVisibleVerticies; }

    FShaderResourceViewRHIRef SRV;
    FUnorderedAccessViewRHIRef UAV;

private:
    uint32 numVertices = 0;
    uint32 visibleVerticies = 0;
};

class VOXELRENDERINGUTILS_API FVoxelVertexTypeBuffer : public FVertexBuffer
{
public:
    FVoxelVertexTypeBuffer() = default;
    ~FVoxelVertexTypeBuffer() = default;
    FVoxelVertexTypeBuffer(uint32 InNumVertices) : numVertices(InNumVertices) {}

    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
    virtual void ReleaseRHI() override;
    uint32 GetVertexCount() const { return numVertices; }

    void SetElementCount(uint32 InNumVertices) {
        numVertices = InNumVertices;
        visibleVerticies = InNumVertices;
    }

    uint32 GetVisibleVerticiesCount() const { return visibleVerticies; }
    void SetVisibleVerticiessCount(uint32 inVisibleVerticies) { visibleVerticies = inVisibleVerticies; }

    FShaderResourceViewRHIRef SRV;
    FUnorderedAccessViewRHIRef UAV;

private:
    uint32 numVertices = 0;
    uint32 visibleVerticies = 0;
};