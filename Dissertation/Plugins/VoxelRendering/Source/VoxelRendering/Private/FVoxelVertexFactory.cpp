#include "FVoxelVertexFactory.h"

class FVoxelVertexFactoryShaderParameters : public FVertexFactoryShaderParameters {
	DECLARE_TYPE_LAYOUT(FVoxelVertexFactoryShaderParameters, NonVirtual);
public:
	void GetElementShaderBindings(
		const FSceneInterface* Scene,
		const FSceneView* View,
		const FMeshMaterialShader* Shader,
		const EVertexInputStreamType InputStreamType,
		ERHIFeatureLevel::Type FeatureLevel,
		const FVertexFactory* VertexFactory,
		const FMeshBatchElement& BatchElement,
		class FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams) const
	{

		FVoxelVertexFactory* VoxelVF = (FVoxelVertexFactory*)VertexFactory;
		const uint32 Index = VoxelVF->FirstIndex;
		//ShaderBindings.Add(TransformIndex, Index);
	}

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		//TransformIndex.Bind(ParameterMap, TEXT("TransformIndex"), SPF_Optional);
		//TransformsSRV.Bind(ParameterMap, TEXT("Transforms"), SPF_Optional);
	};


private:
	//LAYOUT_FIELD(FShaderParameter, TransformIndex);
	//LAYOUT_FIELD(FShaderResourceParameter, TransformsSRV);
};

bool FVoxelVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters) { return false; }

void FVoxelVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
	OutEnvironment.SetDefine(TEXT("VOXEL_MESH"), TEXT("0"));
	FVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);

}
void FVoxelVertexFactory::GetPSOPrecacheVertexFetchElements(EVertexInputStreamType VertexInputStreamType, FVertexDeclarationElementList& Elements) {}

void FVoxelVertexFactory::InitRHI(FRHICommandListBase& RHICmdList){}
void FVoxelVertexFactory::ReleaseRHI() {}

void FVoxelVertexFactory::SetVertexBuffer(const FVertexBuffer* InBuffer, uint32 StreamOffset, uint32 Stride) {}
void FVoxelVertexFactory::SetDynamicParameterBuffer(const FVertexBuffer* InDynamicParameterBuffer, uint32 StreamOffset, uint32 Stride) {}