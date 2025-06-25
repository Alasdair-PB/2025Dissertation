#pragma once
#include "MeshPassProcessor.h"
#include "VoxelPixelMeshMaterialShader.h"
#include "VoxelVertexMeshMaterialShader.h"
#include "VoxelPixelShader.h"
#include "VoxelVertexShader.h"

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

    struct FVoxelElementData : public FMeshMaterialShaderElementData
    {
        const FLightCacheInterface* LCI;
        FVoxelElementData(const FLightCacheInterface* LCI) : LCI(LCI) {}
    };

	void AddMeshBatch(const FMeshBatch& MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId = -1)
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
	}
   
private:
	const FSceneView* View;
    FMeshPassProcessorRenderState DrawRenderState;
};
