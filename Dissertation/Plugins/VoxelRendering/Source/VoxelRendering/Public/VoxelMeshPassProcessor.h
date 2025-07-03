#pragma once
#include "MeshPassProcessor.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "EngineModule.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "GlobalRenderResources.h"
#include "GlobalShader.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "PrimitiveViewRelevance.h"
#include "RHIStaticStates.h"
#include "RenderGraphUtils.h"
#include "SceneInterface.h"
#include "TextureResource.h"
#include "Async/Mutex.h"

#include "VoxelPixelMeshMaterialShader.h"
#include "VoxelVertexMeshMaterialShader.h"
#include "VoxelPixelShader.h"
#include "VoxelVertexShader.h"
#include "RendererInterface.h"
#include "Runtime/Renderer/Private/SceneRendering.h"
#include "MaterialShared.h"

IMPLEMENT_MATERIAL_SHADER_TYPE(, FVoxelPixelMeshMaterialShader, TEXT("/VoxelShaders/VoxelPixelShader.usf"), TEXT("MainPS"), SF_Pixel);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FVoxelVertexMeshMaterialShader, TEXT("/VoxelShaders/VoxelVertexShader.usf"), TEXT("MainVS"), SF_Vertex);

class FVoxelMeshPassProcessor : public FMeshPassProcessor
{
public:
    FVoxelMeshPassProcessor (const FScene* InScene, const FSceneView* InView, FMeshPassDrawListContext* InDrawListContext)
		: FMeshPassProcessor(TEXT("VoxelGBuffer"), InScene, InView->GetFeatureLevel(), InView, InDrawListContext), View(InView) 
    {
        //DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
        //DrawRenderState.SetBlendState(TStaticBlendState<>::GetRHI());
        DrawRenderState.SetDepthStencilAccess(FExclusiveDepthStencil::DepthWrite_StencilWrite);
        DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<true, CF_DepthNearOrEqual>::GetRHI());
        DrawRenderState.SetBlendState(TStaticBlendStateWriteMask<CW_RGBA, CW_RGBA, CW_RGBA, CW_RGBA>::GetRHI());
    }

    bool TryGetMaterialShaders(const FMaterial& InMaterialResource, FMaterialShaders& OutShaders)
    {
        FMaterialShaderTypes ShaderTypes;
        ShaderTypes.AddShaderType<FVoxelVertexMeshMaterialShader>();
        ShaderTypes.AddShaderType<FVoxelPixelMeshMaterialShader>();

        FVertexFactoryType* VertexFactoryType = FindVertexFactoryType(FName(TEXT("FVoxelVertexFactory"), FNAME_Find));
        check(VertexFactoryType != nullptr);
        return InMaterialResource.TryGetShaders(ShaderTypes, VertexFactoryType, OutShaders);
    }

    void AddMeshBatch(const FMeshBatch& MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId = -1)
    {
        const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;
        const FMaterialRenderProxy* FallbackMaterialRenderProxyPtr = nullptr;
        const FMaterial& DefaultMaterial = MeshBatch.MaterialRenderProxy->GetMaterialWithFallback(FeatureLevel, FallbackMaterialRenderProxyPtr);
        const FMaterialRenderProxy& MaterialRenderProxy = FallbackMaterialRenderProxyPtr ? *FallbackMaterialRenderProxyPtr : *MeshBatch.MaterialRenderProxy;

        MaterialRenderProxy.UpdateUniformExpressionCacheIfNeeded(View->FeatureLevel);

        FMaterialShaders Shaders;
        verify(TryGetMaterialShaders(DefaultMaterial, Shaders));
        TMeshProcessorShaders<FVoxelVertexMeshMaterialShader, FVoxelPixelMeshMaterialShader> PassShaders;
        bool bVertexRecieved = Shaders.TryGetVertexShader(PassShaders.VertexShader);
        bool bPixelRecieved = Shaders.TryGetPixelShader(PassShaders.PixelShader);

        check(bVertexRecieved);
        check(bPixelRecieved);

        const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
        const ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(DefaultMaterial, OverrideSettings);
        const ERasterizerCullMode MeshCullMode = ComputeMeshCullMode(DefaultMaterial, OverrideSettings);
        FMeshDrawCommandSortKey SortKey = FMeshDrawCommandSortKey::Default;
        FMeshMaterialShaderElementData ShaderElementData;
        ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, true);

        UE_LOG(LogTemp, Warning, TEXT("Building mesh draw commands pass"));

        BuildMeshDrawCommands(
            MeshBatch,
            BatchElementMask,
            PrimitiveSceneProxy,
            MaterialRenderProxy,
            DefaultMaterial,
            DrawRenderState,
            PassShaders,
            MeshFillMode,
            MeshCullMode,
            SortKey,
            EMeshPassFeatures::Default,
            ShaderElementData);
    }

    struct FVoxelElementData : public FMeshMaterialShaderElementData
    {
        const FLightCacheInterface* LCI;
        FVoxelElementData(const FLightCacheInterface* LCI) : LCI(LCI) {}
    };
   
private:
	const FSceneView* View;
    FMeshPassProcessorRenderState DrawRenderState;
};

/*
FMeshPassProcessor* CreateVoxelPassProcessor(ERHIFeatureLevel::Type FeatureLevel, const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
{
    return new FVoxelMeshPassProcessor(Scene, InViewIfDynamicMeshCommand, InDrawListContext);
}
// VoxelInfoPass added to source file: MeshPassProcessor.h
// 	case EMeshPass::VoxelInfoPass: return TEXT("VoxelInfoPass");
// change 38 to 39
FRegisterPassProcessorCreateFunction RegisterVoxelInfoPass(&CreateVoxelPassProcessor, EShadingPath::Deferred, EMeshPass::VoxelInfoPass, EMeshPassFlags::MainView);
*/


