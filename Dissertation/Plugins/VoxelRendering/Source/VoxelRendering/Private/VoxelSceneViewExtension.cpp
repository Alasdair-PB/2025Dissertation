#include "VoxelSceneViewExtension.h"
#include "VoxelRendering.h"
#include "RenderGraphUtils.h"
#include "SceneTextures.h"
#include "RHIStaticStates.h"
#include "GlobalShader.h"
#include "VoxelVertexShader.h"
#include "VoxelPixelShader.h"

FVoxelSceneViewExtension::FVoxelSceneViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister)
{}

void FVoxelSceneViewExtension::SetSceneProxy(FVoxelSceneProxy* inSceneProxy) {
    sceneProxy = inSceneProxy;
}

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
    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
    GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
    GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

    UE_LOG(LogTemp, Warning, TEXT("Pre-set pipeline"));
    SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
    UE_LOG(LogTemp, Warning, TEXT("Pre-Parameters"));

    FVoxelPixelShader::FParameters PSParams;
    SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PSParams);

    FVoxelVertexShader::FParameters VSParams;
    VSParams.ModelViewProjection = FMatrix44f(MVP);
    SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VSParams);

    uint32 count = VF->GetVertexBuffer()->GetVertexCount();
    UE_LOG(LogTemp, Warning, TEXT("Vertex count %u"), count);


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
