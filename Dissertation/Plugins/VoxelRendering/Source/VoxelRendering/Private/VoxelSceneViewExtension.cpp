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
#include "VoxelCustomRenderPass.h"
#include "SceneView.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "PostProcess/PostProcessInputs.h"
#include "EngineUtils.h"
#include "RHIResourceUtils.h"
#include "Engine/GameInstance.h"

IMPLEMENT_GLOBAL_SHADER(FVoxelPixelShader, "/VoxelShaders/VoxelPixelShader.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FVoxelVertexShader, "/VoxelShaders/VoxelVertexShader.usf", "MainVS", SF_Vertex);

void FVoxelSceneViewExtension::SetSceneProxy(FVoxelSceneProxy* inSceneProxy) {
    sceneProxy = inSceneProxy;
}

void FVoxelSceneViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) {
    const FVector ViewLocation = InView.ViewLocation;
    const TWeakObjectPtr<UWorld> WorldPtr = GetWorld();

    if (!ensureMsgf(WorldPtr.IsValid(), TEXT("FVoxelViewExtension::SetupView was called while it's owning world is not valid! Lifetime of the VoxelViewExtension is tied to the world, this should be impossible!")))
        return;

    int32 NumViews = 1;
    if (WorldPtr->GetGameInstance())
        NumViews = WorldPtr->GetGameInstance()->GetLocalPlayers().Num();

    static bool bUpdatingVoxelInfo = false; // Prevent re-entrancy. 
    if (bUpdatingVoxelInfo) return;

    bUpdatingVoxelInfo = true;
    ON_SCOPE_EXIT{ bUpdatingVoxelInfo = false; };
    FSceneInterface* Scene = WorldPtr.Get()->Scene;
    check(Scene != nullptr);

    UpdateVoxelInfoRendering_CustomRenderPass(Scene, InViewFamily);
}

void FVoxelSceneViewExtension::UpdateVoxelInfoRendering_CustomRenderPass(FSceneInterface* Scene, const FSceneViewFamily& ViewFamily) {

    FSceneInterface::FCustomRenderPassRendererInput PassInput;

    const FScene* Scene = Scene->GetRenderScene();
    FTextureRHIRef RenderTargetTexture = ViewFamily.RenderTarget->GetRenderTargetTexture();
    const FIntPoint RenderTargetSize;
    FVoxelCustomRenderPass* VoxelPass = new FVoxelCustomRenderPass(RenderTargetSize);

    VoxelPass->PerformRenderCapture(FCustomRenderPassBase::ERenderCaptureType::BeginCapture);
    PassInput.CustomRenderPass = VoxelPass;
    TUniquePtr<FMyVoxelRenderData> Data = MakeUnique<FMyVoxelRenderData>(sceneProxy);
    VoxelPass->SetUserData(MoveTemp(Data));

    Scene->AddCustomRenderPass(&ViewFamily, PassInput);
}

void FVoxelSceneViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{

}

void FVoxelSceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
    if (!sceneProxy) return;

#if true
    const FScene* Scene = View.Family->Scene->GetRenderScene();



#elif false
    //----------------- Attempt 1 using Pass processor to render a custom pass from mesh batch data (this may need to be called elsewhere)
    GraphBuilder.AddPass(
        RDG_EVENT_NAME("VoxelRenderPass"),
        ERDGPassFlags::None,
        [this, &View](FRHICommandListImmediate& RHICmdList)
        {
            const FScene* Scene = View.Family->Scene->GetRenderScene();
            FTextureRHIRef RenderTargetTexture = View.Family->RenderTarget->GetRenderTargetTexture();

            if (!Scene) return;
            if (!RenderTargetTexture) return;

            sceneProxy->RenderMyCustomPass(RHICmdList, Scene, &View);
        });

#elif false
    //----------------- Attempt 1 using Global shader DrawIndexedPrimitive to render data as post process
    // ended with Shader Compliation failures are fatal. 
    // It seems like FGlobalShaders do not support vertex information so need to change to FVoxelPixelMeshMaterialShader instead

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
#endif
}
