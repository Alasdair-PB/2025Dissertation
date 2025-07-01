#include "FVoxelVertexFactory.h"
#include "VoxelRenderingUtils.h"
#include "MaterialDomain.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RHIResourceUtils.h"

//IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FVoxelVertexFactoryUniformParameters, "VoxelVF");
//IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FVoxelComputeFactoryUniformParameters, "VoxelCF");

/*#define BINDPARAM(Name) Name.Bind(ParameterMap, TEXT(#Name))
#define SETPARAM(Name) if (Name.IsBound()) { ShaderBindings.Add(Name, UserData->Name); }
#define SETSRVPARAM(Name) if(UserData->Name) { SETPARAM(Name) }*/

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
		const FVoxelBatchElementUserData* UserData = (const FVoxelBatchElementUserData*)BatchElement.UserData;

		if (VoxelVF.IsBound())
			ShaderBindings.Add(VoxelVF, VoxelVertexFactory->GetVertexSRV());

		//FVoxelVertexFactory* VoxelVertexF = (FVoxelVertexFactory*)InVertexFactory;
		//FRHIUniformBuffer* VertexFactoryUniformBuffer = static_cast<FRHIUniformBuffer*>(BatchElement.VertexFactoryUserData);
	}

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		VoxelVF.Bind(ParameterMap, TEXT("VoxelVF"));
		//BINDPARAM(VoxelVF);
		//BINDPARAM(isoLevel);
	};

private:
	LAYOUT_FIELD(FShaderResourceParameter, VoxelVF);	
	//LAYOUT_FIELD(FShaderParameter, voxelsPerAxis);
};
IMPLEMENT_TYPE_LAYOUT(FVoxelVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Vertex, FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Compute, FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Pixel, FVoxelVertexFactoryShaderParameters);

void FVoxelIndexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
{
	uint32 Stride = sizeof(uint32);
	uint32 Size = Stride * numIndices;
	UE_LOG(LogTemp, Warning, TEXT("Indice count %d"), numIndices);

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
	uint32 stride = sizeof(FVector);
	uint32 Size = stride * numVertices;
	FRHIResourceCreateInfo CreateInfo(TEXT("FVoxelVertexBuffer"));
	EBufferUsageFlags UsageFlags = BUF_UnorderedAccess | BUF_ShaderResource | BUF_VertexBuffer;
	VertexBufferRHI = RHICmdList.CreateBuffer(Size, UsageFlags, 0, ERHIAccess::VertexOrIndexBuffer, CreateInfo);

	void* LockedData = RHICmdList.LockBuffer(VertexBufferRHI, 0, Size, RLM_WriteOnly);
	FMemory::Memzero(LockedData, Size);
	RHICmdList.UnlockBuffer(VertexBufferRHI);

	{
		VertexBufferRHI = RHICmdList.CreateVertexBuffer(Size, UsageFlags, CreateInfo);
		if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
		{
			SRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, sizeof(float), PF_R32_FLOAT);
			UAV = RHICmdList.CreateUnorderedAccessView(VertexBufferRHI, PF_R32_FLOAT);
		}
	}
}

FVoxelVertexFactory::FVoxelVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, uint32 bufferSize) : FVertexFactory(InFeatureLevel) //, FVoxelVertexFactoryParameters UniformParams
{
	vertexBuffer.SetElementCount(bufferSize);
	indexBuffer.SetElementCount(bufferSize);
	normalsBuffer.SetElementCount(bufferSize);
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
	//| EVertexFactoryFlags::SupportsPrimitiveIdStream
);

/*
EVertexFactoryFlags::UsedWithMaterials
| EVertexFactoryFlags::SupportsStaticLighting
| EVertexFactoryFlags::SupportsDynamicLighting
| EVertexFactoryFlags::SupportsPrecisePrevWorldPos
| EVertexFactoryFlags::SupportsPositionOnly
| EVertexFactoryFlags::SupportsCachingMeshDrawCommands
| EVertexFactoryFlags::SupportsPrimitiveIdStream
| EVertexFactoryFlags::SupportsRayTracing
| EVertexFactoryFlags::SupportsRayTracingDynamicGeometry
| EVertexFactoryFlags::SupportsLightmapBaking
| EVertexFactoryFlags::SupportsManualVertexFetch
| EVertexFactoryFlags::SupportsPSOPrecaching
| EVertexFactoryFlags::SupportsGPUSkinPassThrough
| EVertexFactoryFlags::SupportsLumenMeshCards*/

struct FDynamicVoxelMeshDataType
{
	FVertexStreamComponent PositionComponent;
	FVertexStreamComponent TangentBasisComponents[2];
	TArray<FVertexStreamComponent, TFixedAllocator<MAX_STATIC_TEXCOORDS / 2> > TextureCoordinates;
	FVertexStreamComponent LightMapCoordinateComponent;
	FVertexStreamComponent ColorComponent;
	FRHIShaderResourceView* PositionComponentSRV = nullptr;
	FRHIShaderResourceView* TangentsSRV = nullptr;
	FRHIShaderResourceView* TextureCoordinatesSRV = nullptr;
	FRHIShaderResourceView* ColorComponentsSRV = nullptr;

	uint32 ColorIndexMask = ~0u;
	int8 LightMapCoordinateIndex = -1;
	uint8 NumTexCoords = 0;
	uint8 LODLightmapDataIndex = 0;
};

void FVoxelVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
{		
	indexBuffer.InitResource(RHICmdList);
	vertexBuffer.InitResource(RHICmdList);
	normalsBuffer.InitResource(RHICmdList);

	FVertexDeclarationElementList Elements;
	Elements.Add(AccessStreamComponent(FVertexStreamComponent(&vertexBuffer, 0, sizeof(FVector3f), VET_Float3), 0));
	Elements.Add(AccessStreamComponent(FVertexStreamComponent(&normalsBuffer, 0, sizeof(FVoxelVertexInfo), VET_Float3), 1));
	bInitialized = true;

	/*FVertexStreamComponent NullTangentXComponent(&GNullVertexBuffer, 0, 0, VET_PackedNormal);
	FVertexStreamComponent NullTangentZComponent(&GNullVertexBuffer, 0, 0, VET_PackedNormal);
	FVertexStreamComponent NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color);
	FVertexStreamComponent NullTexCoordComponent(&GNullVertexBuffer, 0, 0, VET_Float2);

	EVertexInputStreamType VertexStreams = EVertexInputStreamType::Default;
	Elements.Add(AccessStreamComponent(NullTangentXComponent, 1, VertexStreams));
	Elements.Add(AccessStreamComponent(NullTangentZComponent, 2, VertexStreams));
	Elements.Add(AccessStreamComponent(NullColorComponent, 3, VertexStreams));	// Color
	Elements.Add(AccessStreamComponent(NullTexCoordComponent, 4, VertexStreams));	// TexCoords (Unreal expects at least 1)
	Elements.Add(AccessStreamComponent(NullTexCoordComponent, 15, VertexStreams));*/	// LightMap (expected by some PSOs)
	//Elements.Add(AccessStreamComponent(FVertexStreamComponent(nullptr, 0, 0, VET_UInt), 1));
	InitDeclaration(Elements);
}

void FVoxelVertexFactory::ReleaseRHI()
{	
	FVertexFactory::ReleaseRHI();
	vertexBuffer.ReleaseRHI();
	indexBuffer.ReleaseRHI();
	normalsBuffer.ReleaseRHI();
}

