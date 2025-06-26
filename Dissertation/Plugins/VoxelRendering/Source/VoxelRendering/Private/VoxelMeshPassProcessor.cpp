/*#include "VoxelMeshPassProcessor.h"

IMPLEMENT_MATERIAL_SHADER_TYPE(, FVoxelPixelMeshMaterialShader, TEXT("/VoxelShaders/VoxelPixelShader.usf"), TEXT("MainPS"), SF_Pixel);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FVoxelVertexMeshMaterialShader, TEXT("/VoxelShaders/VoxelVertexShader.usf"), TEXT("MainVS"), SF_Vertex);

void FVoxelMeshPassProcessor::AddMeshBatch(const FMeshBatch& MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId)
{
    const FMaterialRenderProxy& DefaultProxy = *UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
    const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;

    const FMaterialRenderProxy* FallbackMaterialRenderProxyPtr = nullptr;
    const FMaterial& DefaultMaterial = MeshBatch.MaterialRenderProxy->GetMaterialWithFallback(FeatureLevel, FallbackMaterialRenderProxyPtr);
    const FMaterialRenderProxy& MaterialRenderProxy = FallbackMaterialRenderProxyPtr ? *FallbackMaterialRenderProxyPtr : *MeshBatch.MaterialRenderProxy;
    TMeshProcessorShaders<FVoxelVertexMeshMaterialShader, FVoxelPixelMeshMaterialShader> voxelPassShaders;

    voxelPassShaders.VertexShader = DefaultMaterial.GetShader<FVoxelVertexMeshMaterialShader>(VertexFactory->GetType());
    voxelPassShaders.PixelShader = DefaultMaterial.GetShader<FVoxelPixelMeshMaterialShader>(VertexFactory->GetType());

    const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
    const ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(DefaultMaterial, OverrideSettings);
    const ERasterizerCullMode MeshCullMode = ComputeMeshCullMode(DefaultMaterial, OverrideSettings);
    FMeshDrawCommandSortKey SortKey = FMeshDrawCommandSortKey::Default;
    FMeshMaterialShaderElementData ShaderElementData;
    ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, true);

    BuildMeshDrawCommands(
        MeshBatch,
        BatchElementMask,
        PrimitiveSceneProxy,
        MaterialRenderProxy,
        DefaultMaterial,
        DrawRenderState,
        voxelPassShaders,
        MeshFillMode,
        MeshCullMode,
        SortKey,
        EMeshPassFeatures::Default,
        ShaderElementData);
}*/

//FMeshPassProcessor* CreateVoxelPassProcessor(ERHIFeatureLevel::Type FeatureLevel, const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
//{
	//return new FVoxelMeshPassProcessor(Scene, InViewIfDynamicMeshCommand, InDrawListContext);
//
// VoxelInfoPass added to source file: MeshPassProcessor.h
// 	case EMeshPass::VoxelInfoPass: return TEXT("VoxelInfoPass");
// change 38 to 39
//FRegisterPassProcessorCreateFunction RegisterVoxelInfoPass(&CreateVoxelPassProcessor, EShadingPath::Deferred, EMeshPass::VoxelInfoPass, EMeshPassFlags::MainView);


