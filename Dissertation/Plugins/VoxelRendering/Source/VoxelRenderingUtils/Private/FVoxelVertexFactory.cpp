#include "FVoxelVertexFactory.h"
#include "VoxelRenderingUtils.h"
#include "MaterialDomain.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RHIResourceUtils.h"

FVoxelBatchElementUserData::FVoxelBatchElementUserData() {}

class FVoxelVertexFactoryShaderParameters : public FVertexFactoryShaderParameters {
	DECLARE_TYPE_LAYOUT(FVoxelVertexFactoryShaderParameters, NonVirtual);
public:
	void GetElementShaderBindings(
		const class FSceneInterface* Scene,
		const class FSceneView* View,
		const class FMeshMaterialShader* Shader,
		const EVertexInputStreamType InputStreamType,
		ERHIFeatureLevel::Type FeatureLevel,
		const class FVertexFactory* InVertexFactory,
		const struct FMeshBatchElement& BatchElement,
		class FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams) const
	{
		const FVoxelVertexFactory* VoxelVertexFactory = static_cast<const FVoxelVertexFactory*>(InVertexFactory);
		const FVoxelBatchElementUserData* UserData = (const FVoxelBatchElementUserData*) BatchElement.UserData;

		if (VoxelVF.IsBound()) {
			ShaderBindings.Add(VoxelVF, VoxelVertexFactory->GetVertexSRV());
			ShaderBindings.Add(VoxelVF, VoxelVertexFactory->GetVertexNormalsSRV());
		}
	}

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		VoxelVF.Bind(ParameterMap, TEXT("VoxelVF"));
		VoxelVF.Bind(ParameterMap, TEXT("VoxelNormalVF"));
	};

private:
	LAYOUT_FIELD(FShaderResourceParameter, VoxelVF);	
	LAYOUT_FIELD(FShaderResourceParameter, VoxelNormalVF);

};
IMPLEMENT_TYPE_LAYOUT(FVoxelVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Vertex, FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Compute, FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Pixel, FVoxelVertexFactoryShaderParameters);

void FVoxelIndexBuffer::ReleaseRHI() {
	FIndexBuffer::ReleaseRHI();
	SRV.SafeRelease();
}

void FVoxelIndexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
{
	uint32 Stride = sizeof(uint32);
	uint32 Size = Stride * numIndices;

	FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelIndexBuffer"));
	EBufferUsageFlags UsageFlags = BUF_Static | BUF_ShaderResource | BUF_IndexBuffer;

	IndexBufferRHI = RHICmdList.CreateBuffer(Size, UsageFlags, 0, ERHIAccess::VertexOrIndexBuffer, CreateInfo);

	void* LockedData = RHICmdList.LockBuffer(IndexBufferRHI, 0, Size, RLM_WriteOnly);
	uint32* IndexData = reinterpret_cast<uint32*>(LockedData);

	for (uint32 i = 0; i < numIndices; i += 3)
	{
		IndexData[i + 0] = i + 2;
		IndexData[i + 1] = i + 1;
		IndexData[i + 2] = i + 0;
	}

	RHICmdList.UnlockBuffer(IndexBufferRHI);
	SRV = RHICmdList.CreateShaderResourceView(IndexBufferRHI, Stride, PF_R32_UINT);
}

void FVoxelVertexBuffer::ReleaseRHI() {
	FVertexBuffer::ReleaseRHI();
	SRV.SafeRelease();
	UAV.SafeRelease();
}

void FVoxelVertexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
{
	uint32 stride = sizeof(FVector);
	uint32 Size = stride * numVertices;
	FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelVertexBuffer"));
	EBufferUsageFlags UsageFlags = BUF_UnorderedAccess | BUF_ShaderResource | BUF_VertexBuffer;
	//VertexBufferRHI = RHICmdList.CreateBuffer(Size, UsageFlags, 0, ERHIAccess::VertexOrIndexBuffer, CreateInfo);
	VertexBufferRHI = RHICmdList.CreateVertexBuffer(Size, UsageFlags, CreateInfo);

	void* LockedData = RHICmdList.LockBuffer(VertexBufferRHI, 0, Size, RLM_WriteOnly);
	FMemory::Memzero(LockedData, Size);
	RHICmdList.UnlockBuffer(VertexBufferRHI);

	if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
	{
		SRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, sizeof(float), PF_R32_FLOAT);
		UAV = RHICmdList.CreateUnorderedAccessView(VertexBufferRHI, PF_R32_FLOAT);
	}
}

FVoxelVertexFactory::FVoxelVertexFactory(uint32 bufferSize) : FVertexFactory(ERHIFeatureLevel::SM5)
{
	vertexBuffer.SetElementCount(bufferSize);
	indexBuffer.SetElementCount(bufferSize);
	normalsBuffer.SetElementCount(bufferSize);
}

void FVoxelVertexFactory::Initialize(uint32 bufferSize)
{
	vertexBuffer.SetElementCount(bufferSize);
	indexBuffer.SetElementCount(bufferSize);
	normalsBuffer.SetElementCount(bufferSize);
	InitResource(FRHICommandListImmediate::Get());
}

bool FVoxelVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

void FVoxelVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
	FVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.SetDefine(TEXT("VOXEL_MESH"), TEXT("1"));
	OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_PRIMITIVE_SCENE_DATA"), Parameters.VertexFactoryType->SupportsPrimitiveIdStream());
	OutEnvironment.SetDefine(TEXT("RAY_TRACING_DYNAMIC_MESH_IN_LOCAL_SPACE"), TEXT("1"));
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FVoxelVertexFactory, "/VertexFactoryShaders/VoxelVertexFactory.ush",
	EVertexFactoryFlags::UsedWithMaterials
	| EVertexFactoryFlags::SupportsDynamicLighting
	| EVertexFactoryFlags::SupportsPositionOnly
	| EVertexFactoryFlags::SupportsRayTracingDynamicGeometry
);

void FVoxelVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
{		
	indexBuffer.InitResource(RHICmdList);
	vertexBuffer.InitResource(RHICmdList);
	normalsBuffer.InitResource(RHICmdList);

	FVertexDeclarationElementList Elements;
	Elements.Add(AccessStreamComponent(FVertexStreamComponent(&vertexBuffer, 0, sizeof(FVector3f), VET_Float3), 0));
	Elements.Add(AccessStreamComponent(FVertexStreamComponent(&normalsBuffer, 0, sizeof(FVoxelVertexInfo), VET_Float3), 1));
	bInitialized = true;
	InitDeclaration(Elements);
}

void FVoxelVertexFactory::ReleaseRHI()
{	
	FVertexFactory::ReleaseRHI();
	vertexBuffer.ReleaseRHI();
	indexBuffer.ReleaseRHI();
	normalsBuffer.ReleaseRHI();
}

