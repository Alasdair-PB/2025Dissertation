#include "FVoxelVertexFactory.h"
#include "MaterialDomain.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RHIResourceUtils.h"

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FVoxelVertexFactoryParameters, "VoxelVF");
#define BINDPARAM(Name) Name.Bind(ParameterMap, TEXT(#Name))
#define SETPARAM(Name) if (Name.IsBound()) { ShaderBindings.Add(Name, UserData->Name); }
#define SETSRVPARAM(Name) if(UserData->Name) { SETPARAM(Name) }

FVoxelBatchElementUserData::FVoxelBatchElementUserData()
	: voxelsPerAxis(0)
	, baseDepthScale(0)
	, isoLevel(0)
{}

class FVoxelVertexFactoryShaderParameters : public FVertexFactoryShaderParameters {
	DECLARE_TYPE_LAYOUT(FVoxelVertexFactoryShaderParameters, NonVirtual);
public:
	LAYOUT_FIELD(FShaderParameter, voxelsPerAxis);
	LAYOUT_FIELD(FShaderParameter, baseDepthScale);
	LAYOUT_FIELD(FShaderParameter, isoLevel);

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
		FVoxelVertexFactory* VoxelVF = (FVoxelVertexFactory*)InVertexFactory;
		FRHIUniformBuffer* VertexFactoryUniformBuffer = static_cast<FRHIUniformBuffer*>(BatchElement.VertexFactoryUserData);
		FVoxelBatchElementUserData* UserData = (FVoxelBatchElementUserData*)BatchElement.UserData;

		SETSRVPARAM(isoLevel);
		SETSRVPARAM(baseDepthScale);
		SETSRVPARAM(voxelsPerAxis);

		if (VertexFactoryUniformBuffer)
			ShaderBindings.Add(Shader->GetUniformBufferParameter<FVoxelVertexFactoryShaderParameters>(), VertexFactoryUniformBuffer);
	}

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		BINDPARAM(isoLevel);
		BINDPARAM(baseDepthScale);
		BINDPARAM(voxelsPerAxis);
	};


private:
	//LAYOUT_FIELD(FShaderResourceParameter, MySRV);
};

IMPLEMENT_TYPE_LAYOUT(FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Vertex, FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Compute, FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Pixel, FVoxelVertexFactoryShaderParameters);

void FVoxelIndexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
{
	uint32 Size = sizeof(uint32) * numIndices;
	FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelIndexBuffer"));
	EBufferUsageFlags UsageFlags = BUF_UnorderedAccess | BUF_ShaderResource | BUF_IndexBuffer;
	const ERHIAccess InitialState = ERHIAccess::UAVCompute | ERHIAccess::SRVCompute;

	IndexBufferRHI = RHICmdList.CreateBuffer(Size, UsageFlags, 0, InitialState, CreateInfo);
	void* LockedData = RHICmdList.LockBuffer(IndexBufferRHI, 0, Size, RLM_WriteOnly);
	FMemory::Memzero(LockedData, Size); // FMemory::Memset(LockedData, 0xFF, Size);
	RHICmdList.UnlockBuffer(IndexBufferRHI);
}

void FVoxelVertexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
{
	uint32 stride = sizeof(FVoxelVertexInfo);
	uint32 Size = stride * numVertices;
	FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelVertexBuffer"));
	EBufferUsageFlags UsageFlags = BUF_UnorderedAccess | BUF_ShaderResource | BUF_VertexBuffer;
	const ERHIAccess InitialState = ERHIAccess::UAVCompute | ERHIAccess::SRVCompute;

	VertexBufferRHI = RHICmdList.CreateBuffer(Size, UsageFlags, 0, InitialState, CreateInfo);
	void* LockedData = RHICmdList.LockBuffer(VertexBufferRHI, 0, Size, RLM_WriteOnly);
	FMemory::Memzero(LockedData, Size);		//FMemory::Memcpy(LockedData, Vertices.GetData(), Size);
	RHICmdList.UnlockBuffer(VertexBufferRHI);

	SRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, Size, PF_R32_FLOAT);

	FVoxelVertexFactoryParameters VSParams;
	VSParams.VertexFetch_Buffer = SRV;
	VertexUniformBuffer = TUniformBufferRef<FVoxelVertexFactoryParameters>::CreateUniformBufferImmediate(VSParams, UniformBuffer_MultiFrame);

	FVoxelComputeFactoryParameters CSParams;
	FUnorderedAccessViewRHIRef VertexUAV = RHICmdList.CreateUnorderedAccessView(VertexBufferRHI, false, false);
	CSParams.VertexFetch_Buffer = VertexUAV;
	ComputeUniformBuffer =TUniformBufferRef<FVoxelComputeFactoryParameters>::CreateUniformBufferImmediate(CSParams, UniformBuffer_SingleFrame);

}

bool FVoxelVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

void FVoxelVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
	OutEnvironment.SetDefine(TEXT("VOXEL_MESH"), TEXT("1"));
	OutEnvironment.SetDefine(TEXT("RAY_TRACING_DYNAMIC_MESH_IN_LOCAL_SPACE"), TEXT("1"));
	FVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FVoxelVertexFactory, "/VertexFactoryShaders/VoxelVertexFactory.ush",
	EVertexFactoryFlags::UsedWithMaterials
	| EVertexFactoryFlags::SupportsDynamicLighting
	| EVertexFactoryFlags::SupportsPositionOnly
	| EVertexFactoryFlags::SupportsRayTracingDynamicGeometry
);

void FVoxelVertexFactory::GetPSOPrecacheVertexFetchElements(EVertexInputStreamType VertexInputStreamType, FVertexDeclarationElementList& Elements)
{
	const uint32 Stride = sizeof(FVoxelVertexInfo);
	Elements.Add(FVertexElement(0, STRUCT_OFFSET(FVoxelVertexInfo, Position), VET_Float3, 0, Stride)); // POSITION
	Elements.Add(FVertexElement(0, STRUCT_OFFSET(FVoxelVertexInfo, Normal), VET_Float3, 1, Stride));   // NORMAL
}

void FVoxelVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
{	
	vertexBuffer.InitResource(RHICmdList);
	indexBuffer.InitResource(RHICmdList);
	FVertexDeclarationElementList Elements;
	GetPSOPrecacheVertexFetchElements(EVertexInputStreamType::Default, Elements);
	InitDeclaration(Elements);

	FVoxelVertexFactoryShaderParameters UniformParameters;
	UniformParameters.VertexFetch_Buffer = GetVertexBufferSRV();
	if (UniformParameters.VertexFetch_Buffer)
	{
		UniformBuffer = TUniformBufferRef<FLidarPointCloudVertexFactoryUniformShaderParameters>::CreateUniformBufferImmediate(UniformParameters, UniformBuffer_MultiFrame);
	}
}

void FVoxelVertexFactory::ReleaseRHI()
{
	vertexBuffer.ReleaseRHI();
	indexBuffer.ReleaseRHI();
	FVertexFactory::ReleaseRHI();
}

