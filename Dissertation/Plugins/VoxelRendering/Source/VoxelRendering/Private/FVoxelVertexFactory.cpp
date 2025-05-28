#include "FVoxelVertexFactory.h"
#include "MaterialDomain.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RHIResourceUtils.h"

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
		FVoxelVertexFactory* VoxelVF = (FVoxelVertexFactory*)InVertexFactory;
		const uint32 Index = VoxelVF->firstIndex;
		//ShaderBindings.Add(Shader->GetUniformBufferParameter<FVoxelVertexFactoryParameters>(), VoxelVF->UniformBuffer);
	}

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		//MySRV.Bind(ParameterMap, TEXT("Data"), SPF_Optional);
	};


private:
	//LAYOUT_FIELD(FShaderResourceParameter, MySRV);
};

IMPLEMENT_TYPE_LAYOUT(FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactoryShaderParameters, SF_Vertex, FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactoryShaderParameters, SF_Pixel, FVoxelVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_TYPE(FVoxelVertexFactory, "/VertexFactoryShaders/VoxelVertexFactory.ush",
	EVertexFactoryFlags::UsedWithMaterials
	| EVertexFactoryFlags::SupportsDynamicLighting
	| EVertexFactoryFlags::SupportsPositionOnly
	| EVertexFactoryFlags::SupportsRayTracingDynamicGeometry
);

bool FVoxelVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters) { return true; }

void FVoxelVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
	OutEnvironment.SetDefine(TEXT("VOXEL_MESH"), TEXT("1"));
	FVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);
}

void FVoxelVertexFactory::GetPSOPrecacheVertexFetchElements(EVertexInputStreamType VertexInputStreamType, FVertexDeclarationElementList& Elements)
{
	const uint32 Stride = sizeof(FVoxelVertexInfo);
	Elements.Add(FVertexElement(0, STRUCT_OFFSET(FVoxelVertexInfo, Position), VET_Float3, 0, Stride)); // POSITION
	Elements.Add(FVertexElement(0, STRUCT_OFFSET(FVoxelVertexInfo, Normal), VET_Float3, 1, Stride));   // NORMAL
}

void FVoxelVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
{
	FVertexDeclarationElementList Elements;
	GetPSOPrecacheVertexFetchElements(EVertexInputStreamType::Default, Elements);
	InitDeclaration(Elements);
}

void FVoxelVertexFactory::ReleaseRHI()
{
	FVertexFactory::ReleaseRHI();
}

