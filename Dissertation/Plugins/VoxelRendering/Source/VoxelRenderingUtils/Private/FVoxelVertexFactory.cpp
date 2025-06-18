#include "FVoxelVertexFactory.h"
#include "VoxelRenderingUtils.h"
#include "MaterialDomain.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RHIResourceUtils.h"

//IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FVoxelVertexFactoryUniformParameters, "VoxelVF");
//IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FVoxelComputeFactoryUniformParameters, "VoxelCF");

#define BINDPARAM(Name) Name.Bind(ParameterMap, TEXT(#Name))
#define SETPARAM(Name) if (Name.IsBound()) { ShaderBindings.Add(Name, UserData->Name); }
#define SETSRVPARAM(Name) if(UserData->Name) { SETPARAM(Name) }

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
		FVoxelVertexFactory* VoxelVertexF = (FVoxelVertexFactory*)InVertexFactory;
		FRHIUniformBuffer* VertexFactoryUniformBuffer = static_cast<FRHIUniformBuffer*>(BatchElement.VertexFactoryUserData);
		FVoxelBatchElementUserData* UserData = (FVoxelBatchElementUserData*)BatchElement.UserData;

		//SETSRVPARAM(isoLevel);
		//if (VertexFactoryUniformBuffer)
		//	ShaderBindings.Add(Shader->GetUniformBufferParameter<FVoxelVertexFactoryShaderParameters>(), VertexFactoryUniformBuffer);

		ShaderBindings.Add(VoxelVF, VoxelVertexF->GetVertexSRV());
	}

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		BINDPARAM(VoxelVF);
		//BINDPARAM(isoLevel);
		//BINDPARAM(baseDepthScale);
		//BINDPARAM(voxelsPerAxis);
	};

private:
	LAYOUT_FIELD(FShaderResourceParameter, VoxelVF);	
	//LAYOUT_FIELD(FShaderParameter, voxelsPerAxis);
	//LAYOUT_FIELD(FShaderParameter, baseDepthScale);
	//LAYOUT_FIELD(FShaderParameter, isoLevel);
};

IMPLEMENT_TYPE_LAYOUT(FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Vertex, FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Compute, FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Pixel, FVoxelVertexFactoryShaderParameters);

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

void FVoxelVertexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
{
	uint32 stride = sizeof(FVoxelVertexInfo);
	uint32 Size = stride * numVertices;
	FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelVertexBuffer"));
	EBufferUsageFlags UsageFlags = BUF_UnorderedAccess | BUF_ShaderResource | BUF_VertexBuffer;

	VertexBufferRHI = RHICmdList.CreateStructuredBuffer(stride, Size, UsageFlags, CreateInfo);
	void* LockedData = RHICmdList.LockBuffer(VertexBufferRHI, 0, Size, RLM_WriteOnly);
	FMemory::Memzero(LockedData, Size);
	RHICmdList.UnlockBuffer(VertexBufferRHI);

	SRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI);
	UAV = RHICmdList.CreateUnorderedAccessView(VertexBufferRHI, false, false);
}

FVoxelVertexFactory::FVoxelVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, uint32 bufferSize) : FVertexFactory(InFeatureLevel) //, FVoxelVertexFactoryParameters UniformParams
{
	vertexBuffer.SetElementCount(bufferSize);
	indexBuffer.SetElementCount(bufferSize);
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

	// Will look at enabling support for these features once core has been established
	//| EVertexFactoryFlags::SupportsDynamicLighting
	//| EVertexFactoryFlags::SupportsPositionOnly
	//| EVertexFactoryFlags::SupportsRayTracingDynamicGeometry
);

void FVoxelVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
{		
	indexBuffer.InitResource(RHICmdList);
	vertexBuffer.InitResource(RHICmdList);

	FVertexDeclarationElementList Elements;
	Elements.Add(AccessStreamComponent(FVertexStreamComponent(&vertexBuffer, STRUCT_OFFSET(FVoxelVertexInfo, Position), sizeof(FVoxelVertexInfo), VET_Float3), 0));
	Elements.Add(AccessStreamComponent(FVertexStreamComponent(&vertexBuffer, STRUCT_OFFSET(FVoxelVertexInfo, Normal), sizeof(FVoxelVertexInfo), VET_Float3), 0));
	InitDeclaration(Elements);
}

void FVoxelVertexFactory::ReleaseRHI()
{
	vertexBuffer.ReleaseRHI();
	indexBuffer.ReleaseRHI();
	FVertexFactory::ReleaseRHI();
}

