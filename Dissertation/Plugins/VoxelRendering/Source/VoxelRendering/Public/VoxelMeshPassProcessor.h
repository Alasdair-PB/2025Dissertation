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

//IMPLEMENT_GLOBAL_SHADER(FVoxelPixelShader, "/VoxelShaders/VoxelPixelShader.usf", "MainPS", SF_Pixel);
//IMPLEMENT_GLOBAL_SHADER(FVoxelVertexShader, "/VoxelShaders/VoxelVertexShader.usf", "MainVS", SF_Vertex);
//IMPLEMENT_MATERIAL_SHADER_TYPE(, FVoxelPixelMeshMaterialShader, TEXT("/VoxelShaders/VoxelPixelShader.usf"), TEXT("MainPS"), SF_Pixel);
//IMPLEMENT_MATERIAL_SHADER_TYPE(, FVoxelVertexMeshMaterialShader, TEXT("/VoxelShaders/VoxelVertexShader.usf"), TEXT("MainVS"), SF_Vertex);

/*
class FMyMeshPassProcessor : public FMeshPassProcessor
{
public:
	FMyMeshPassProcessor(const FScene* InScene, const FSceneView* InView, FMeshPassDrawListContext* InDrawListContext)
		: FMeshPassProcessor(TEXT("VoxelGBuffer"), InScene, InView->GetFeatureLevel(), InView, InDrawListContext), View(InView) 
    {
        DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
        DrawRenderState.SetBlendState(TStaticBlendState<>::GetRHI());
    }

    struct FVoxelElementData : public FMeshMaterialShaderElementData
    {
        const FLightCacheInterface* LCI;
        FVoxelElementData(const FLightCacheInterface* LCI) : LCI(LCI) {}
    };

	void AddMeshBatch(const FMeshBatch& MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId = -1)
	{
        const FMaterialRenderProxy& DefaultProxy = *UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
        const FMaterial& DefaultMaterial = *DefaultProxy.GetMaterialNoFallback(FeatureLevel);

		//TShaderRef<FVoxelVertexMeshMaterialShader> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		//TShaderRef<FVoxelPixelMeshMaterialShader> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
        TMeshProcessorShaders<FVoxelVertexMeshMaterialShader, FVoxelVertexMeshMaterialShader> Shaders;

        const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
        const ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(DefaultMaterial, OverrideSettings);
        const ERasterizerCullMode MeshCullMode = ComputeMeshCullMode(DefaultMaterial, OverrideSettings);
        const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;

        FMeshDrawCommandSortKey SortKey{};
        FVoxelElementData ShaderElementData(MeshBatch.LCI);

        BuildMeshDrawCommands(
            MeshBatch,
            BatchElementMask,
            PrimitiveSceneProxy,
            DefaultProxy,
            DefaultMaterial,
            DrawRenderState,
            Shaders,
            MeshFillMode,
            MeshCullMode,
            SortKey,
            EMeshPassFeatures::Default,
            ShaderElementData);
	}

private:
	const FSceneView* View;
    FMeshPassProcessorRenderState DrawRenderState;
};*/