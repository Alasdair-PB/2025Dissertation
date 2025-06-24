#include "VoxelSceneViewExtension.h"
#include "RenderGraphUtils.h"
#include "SceneTextures.h"
#include "RHIStaticStates.h"
#include "GlobalShader.h"
#include "VoxelVertexShader.h"
#include "VoxelPixelShader.h"

FVoxelSceneViewExtension::FVoxelSceneViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister)
{}

void FVoxelSceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
    if (!sceneProxy) return;
	FSceneViewExtensionBase::PrePostProcessPass_RenderThread(GraphBuilder, View, Inputs);

    const FMatrix ViewMatrix = View.ViewMatrices.GetViewMatrix();
    const FMatrix ProjectionMatrix = View.ViewMatrices.GetProjectionMatrix();
    const FMatrix ViewProj = ViewMatrix * ProjectionMatrix;
    const FMatrix ltw = sceneProxy->GetLocalToWorld();
    const FMatrix MVP = ltw * ViewProj;

    FVoxelVertexFactory* VF = sceneProxy->GetVertexFactory();

    FTextureRHIRef RenderTargetTexture = View.Family->RenderTarget->GetRenderTargetTexture();
    FRHICommandListImmediate& RHICmdList = GraphBuilder.RHICmdList;

    FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::Load_Store);
    RHICmdList.BeginRenderPass(RPInfo, TEXT("CustomPass"));

    TShaderMapRef<FVoxelVertexShader> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    TShaderMapRef<FVoxelPixelShader> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FGraphicsPipelineStateInitializer PSOInit;
    PSOInit.RenderTargetFormats[0] = GPixelFormats[PF_B8G8R8A8].PlatformFormat;
    for (int32 i = 1; i < MaxSimultaneousRenderTargets; ++i)
        PSOInit.RenderTargetFormats[i] = PF_Unknown;
    PSOInit.BlendState = TStaticBlendState<>::GetRHI();
    PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
    PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

    PSOInit.BoundShaderState.VertexDeclarationRHI = VF->GetDeclaration();
    PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
    PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
    PSOInit.PrimitiveType = PT_TriangleList;

    SetGraphicsPipelineState(RHICmdList, PSOInit, 0);

    FVoxelVertexShader::FParameters VSParams;
    VSParams.ModelViewProjection = FMatrix44f(MVP);
    SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VSParams);

    FVoxelPixelShader::FParameters PSParams;
    SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PSParams);

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
