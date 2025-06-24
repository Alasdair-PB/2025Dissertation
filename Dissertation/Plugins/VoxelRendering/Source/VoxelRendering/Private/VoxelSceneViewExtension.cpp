#include "VoxelSceneViewExtension.h"
#include "VoxelRendering.h"
#include "RenderGraphUtils.h"
#include "SceneTextures.h"
#include "RHIStaticStates.h"
#include "GlobalShader.h"
#include "VoxelVertexShader.h"
#include "VoxelPixelShader.h"
#include "VoxelPixelMeshMaterialShader.h"
#include "VoxelVertexMeshMaterialShader.h"

IMPLEMENT_GLOBAL_SHADER(FVoxelPixelShader, "/VoxelShaders/VoxelPixelShader.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FVoxelVertexShader, "/VoxelShaders/VoxelVertexShader.usf", "MainVS", SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FVoxelPixelMeshMaterialShader, TEXT("/VoxelShaders/VoxelPixelShader.usf"), TEXT("MainPS"), SF_Pixel);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FVoxelVertexMeshMaterialShader, TEXT("/VoxelShaders/VoxelVertexShader.usf"), TEXT("MainVS"), SF_Vertex);

FVoxelSceneViewExtension::FVoxelSceneViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister)
{}

void FVoxelSceneViewExtension::SetSceneProxy(FVoxelSceneProxy* inSceneProxy) {
    sceneProxy = inSceneProxy;
}

//* 
// It seems like FGlobalShaders do not support vertex information so need to change to FVoxelPixelMeshMaterialShader instead

void FVoxelSceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
    if (!sceneProxy) return;

	FSceneViewExtensionBase::PrePostProcessPass_RenderThread(GraphBuilder, View, Inputs);

    const FMatrix ViewMatrix = View.ViewMatrices.GetViewMatrix();
    const FMatrix ProjectionMatrix = View.ViewMatrices.GetProjectionMatrix();
    const FMatrix ViewProj = ViewMatrix * ProjectionMatrix;
    const FMatrix ltw = sceneProxy->GetLocalToWorld();
    const FMatrix MVP =  ViewProj * ltw;

    FVoxelVertexFactory* VF = sceneProxy->GetVertexFactory();

    FTextureRHIRef RenderTargetTexture = View.Family->RenderTarget->GetRenderTargetTexture();
    FRHICommandListImmediate& RHICmdList = GraphBuilder.RHICmdList;

    FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::Load_Store);
    RHICmdList.BeginRenderPass(RPInfo, TEXT("CustomPass"));

    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
    TShaderMapRef<FVoxelVertexShader> VertexShader(GlobalShaderMap);
    TShaderMapRef<FVoxelPixelShader> PixelShader(GlobalShaderMap);

    FGraphicsPipelineStateInitializer GraphicsPSOInit;
    RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
    GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
    GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
    GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
    GraphicsPSOInit.PrimitiveType = PT_TriangleList;
    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VF->GetDeclaration();
    GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
    GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

    SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

    FVoxelVertexShader::FParameters VSParams;
    VSParams.ModelViewProjection = FMatrix44f(MVP);
    SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VSParams);

    FVoxelPixelShader::FParameters PSParams;
    SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PSParams);

    uint32 count = VF->GetVertexBuffer()->GetVertexCount();

    RHICmdList.SetStreamSource(0, VF->GetVertexBuffer()->VertexBufferRHI, 0);
    RHICmdList.DrawIndexedPrimitive(
        VF->GetIndexBufferRHIRef(),
        0,
        0,
        VF->GetVertexBuffer()->GetVertexCount(),
        0,
        VF->GetIndexBuffer()->GetIndexCount() / 3,
        1
    );

    RHICmdList.EndRenderPass();
}
