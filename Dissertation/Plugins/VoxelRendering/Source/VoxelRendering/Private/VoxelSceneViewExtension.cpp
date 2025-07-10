#include "VoxelSceneViewExtension.h"
#include "VoxelRendering.h"
#include "RenderGraphUtils.h"
#include "SceneTextures.h"
#include "RHIStaticStates.h"
#include "GlobalShader.h"
#include "VoxelVertexShader.h"
#include "VoxelPixelShader.h"
#include "VoxelMeshComponent.h"

#include "EngineUtils.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "PostProcess/PostProcessInputs.h"

#include "Rendering/CustomRenderPass.h"
#include "RenderGraphBuilder.h"
#include "SceneView.h"
#include "SceneRendering.h"

#include "ScreenPass.h"
#include "SystemTextures.h"
//#include "VoxelMeshPassProcessor.h"

IMPLEMENT_GLOBAL_SHADER(FVoxelPixelShader, "/VoxelShaders/VoxelPixelShader.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FVoxelVertexShader, "/VoxelShaders/VoxelVertexShader.usf", "MainVS", SF_Vertex);

FScreenPassTextureViewportParameters GetTextureViewportParameters(const FScreenPassTextureViewport& InViewport)
{
    const FVector2f Extent(InViewport.Extent);
    const FVector2f ViewportMin(InViewport.Rect.Min.X, InViewport.Rect.Min.Y);
    const FVector2f ViewportMax(InViewport.Rect.Max.X, InViewport.Rect.Max.Y);
    const FVector2f ViewportSize = ViewportMax - ViewportMin;

    FScreenPassTextureViewportParameters Parameters;

    if (!InViewport.IsEmpty())
    {
        Parameters.Extent = FVector2f(Extent);
        Parameters.ExtentInverse = FVector2f(1.0f / Extent.X, 1.0f / Extent.Y);

        Parameters.ScreenPosToViewportScale = FVector2f(0.5f, -0.5f) * ViewportSize;
        Parameters.ScreenPosToViewportBias = (0.5f * ViewportSize) + ViewportMin;

        Parameters.ViewportMin = InViewport.Rect.Min;
        Parameters.ViewportMax = InViewport.Rect.Max;

        Parameters.ViewportSize = ViewportSize;
        Parameters.ViewportSizeInverse = FVector2f(1.0f / Parameters.ViewportSize.X, 1.0f / Parameters.ViewportSize.Y);

        Parameters.UVViewportMin = ViewportMin * Parameters.ExtentInverse;
        Parameters.UVViewportMax = ViewportMax * Parameters.ExtentInverse;

        Parameters.UVViewportSize = Parameters.UVViewportMax - Parameters.UVViewportMin;
        Parameters.UVViewportSizeInverse = FVector2f(1.0f / Parameters.UVViewportSize.X, 1.0f / Parameters.UVViewportSize.Y);

        Parameters.UVViewportBilinearMin = Parameters.UVViewportMin + 0.5f * Parameters.ExtentInverse;
        Parameters.UVViewportBilinearMax = Parameters.UVViewportMax - 0.5f * Parameters.ExtentInverse;
    }

    return Parameters;
}

void FVoxelSceneViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) {

#if true
    //----------------- Attempt 1 using View extion to call a custom render pass- stopped as custom render path is intended for multi view scenarios like
    // reflections & depth textures. To continue this approach the data for view 1 would need to be rendered to a texture and added to scene output

    const FVector ViewLocation = InView.ViewLocation;
    const TWeakObjectPtr<UWorld> WorldPtr = GetWorld();
    int32 NumViews = 1;

    if (!ensureMsgf(WorldPtr.IsValid(), TEXT("FVoxelViewExtension::SetupView was called while it's owning world is not valid!"))) return;
    if (WorldPtr->GetGameInstance()) NumViews = WorldPtr->GetGameInstance()->GetLocalPlayers().Num();

    static bool bUpdatingVoxelInfo = false;
    if (bUpdatingVoxelInfo) return;

    bUpdatingVoxelInfo = true;
    ON_SCOPE_EXIT{ bUpdatingVoxelInfo = false; };
    FSceneInterface* Scene = WorldPtr.Get()->Scene;
    check(Scene != nullptr);


    for (AVoxelBody* VoxelBody : TActorRange<AVoxelBody>(WorldPtr.Get()))
    {
        if (VoxelBody->HasActorRegisteredAllComponents())
        {
            FVoxelBodyInfo& VoxelBodyInfo = VoxelBodiesInfos.FindOrAdd(VoxelBody);
            VoxelBodyInfo.RenderContext.RenderedBody = VoxelBody;
            VoxelBodyInfo.RenderContext.VoxelBodies.Reset();
            UVoxelMeshComponent* MeshComp = VoxelBody->GetComponentByClass<UVoxelMeshComponent>();

            if (MeshComp && MeshComp->IsRegistered())
                VoxelBodyInfo.RenderContext.VoxelBodies.Add(MeshComp);
          
            int32_t ExpectedNumViews = 4;

            if (VoxelBodyInfo.ViewInfos.Num() != ExpectedNumViews)
                VoxelBodyInfo.ViewInfos.SetNum(ExpectedNumViews);
        }    
    }
#endif
}

#if false
void FVoxelSceneViewExtension::UpdateVoxelInfoRendering_CustomRenderPass(FSceneInterface* Scene, const FSceneViewFamily& ViewFamily, const FVoxelBodyInfo* VoxelBodyInfo) {


    //----------------- Attempt 1 continued -----------------

    const FRenderingContext& Context(VoxelBodyInfo->RenderContext);


    FSceneInterface::FCustomRenderPassRendererInput PassInput;
    PassInput.ViewLocation = ViewLocation;
    PassInput.ViewRotationMatrix = FLookAtMatrix(ViewLocation, LookAt, FVector(0.f, -1.f, 0.f));
    PassInput.ViewRotationMatrix = PassInput.ViewRotationMatrix.RemoveTranslation();
    PassInput.ViewRotationMatrix.RemoveScaling();
    PassInput.ProjectionMatrix = BuildOrthoMatrix(ZoneExtent.X, ZoneExtent.Y);
    PassInput.ViewActor = Context.RenderedBody;	

    TSet<FPrimitiveComponentId> ComponentsToRenderInDepthPass;
    if (Context.VoxelBodies.Num() > 0)
    {
        ComponentsToRenderInDepthPass.Reserve(Context.VoxelBodies.Num());
        for (TWeakObjectPtr<UVoxelMeshComponent> VoxelMeshCom : Context.VoxelBodies)
        {
            if (VoxelMeshCom.IsValid())
                ComponentsToRenderInDepthPass.Add(VoxelMeshCom.Get()->GetPrimitiveSceneId());
        }
    }
    PassInput.ShowOnlyPrimitives = MoveTemp(ComponentsToRenderInDepthPass);

    const FScene* Scene = Scene->GetRenderScene();
    FTextureRHIRef RenderTargetTexture = ViewFamily.RenderTarget->GetRenderTargetTexture();
    const FIntPoint RenderTargetSize;
    FVoxelCustomRenderPass* VoxelPass = new FVoxelCustomRenderPass(RenderTargetSize);

    VoxelPass->PerformRenderCapture(FCustomRenderPassBase::ERenderCaptureType::BeginCapture);
    PassInput.CustomRenderPass = VoxelPass;

    TUniquePtr<FMyVoxelRenderData> Data = MakeUnique<FMyVoxelRenderData>(VoxelBodyInfo->ViewInfos[3].OldSceneProxy);
    //VoxelPass->SetUserData(MoveTemp(Data));

    Scene->AddCustomRenderPass(&ViewFamily, PassInput);
}
#endif

void FVoxelSceneViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& View) {}

BEGIN_SHADER_PARAMETER_STRUCT(FVoxelTargetParameters, )
    RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

void FVoxelSceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass PassId, const FSceneView& InView, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
   // if (PassId == EPostProcessingPass::SSRInput)
   //    InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FVoxelSceneViewExtension::PostProcessPassSSRInput_RenderThread));
}

FScreenPassTexture FVoxelSceneViewExtension::PostProcessPassSSRInput_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
{                
    FScreenPassTexture sceneColor = FScreenPassTexture::CopyFromSlice(GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor));
    check(sceneColor.IsValid());
    FScreenPassRenderTarget output = Inputs.OverrideOutput;
    if (!output.IsValid())
        output = FScreenPassRenderTarget::CreateFromInput(GraphBuilder, sceneColor, View.GetOverwriteLoadAction(), TEXT("VoxelOutput"));

    // Copy
    checkSlow(View.bIsViewInfo);
    const FViewInfo& viewInfo = static_cast<const FViewInfo&>(View);
    const FIntRect PrimaryViewRect = viewInfo.ViewRect;

    FVoxelTargetParameters* PassParameters = GraphBuilder.AllocParameters<FVoxelTargetParameters>();
    PassParameters->RenderTargets[0] = output.GetRenderTargetBinding();

    GraphBuilder.AddPass(
        RDG_EVENT_NAME("VoxelRenderPass"),
        PassParameters,
        ERDGPassFlags::Raster,
        [this, &View, PrimaryViewRect, output](FRHICommandListImmediate& RHICmdList)
        {
            UE_LOG(LogTemp, Warning, TEXT("Start pass"));
            FRHIRenderPassInfo RPInfo(output.Texture->GetRHI(), ERenderTargetActions::Load_Store);
            RHICmdList.SetViewport(
                 PrimaryViewRect.Min.X, PrimaryViewRect.Min.Y, 0.0f,
                 PrimaryViewRect.Max.X, PrimaryViewRect.Max.Y, 1.0f
             );

            for (const auto& Pair : VoxelBodiesInfos)
            {
                const FVoxelBodyInfo& Info = Pair.Value;
                for (const auto& MeshComp : Info.RenderContext.VoxelBodies)
                {
                    if (MeshComp.IsValid() && MeshComp->SceneProxy)
                    {
                        FPrimitiveSceneProxy* sceneProxy = MeshComp->SceneProxy;
                        if (sceneProxy == nullptr) continue;
                        FVoxelSceneProxy* VoxelSceneProxy = static_cast<FVoxelSceneProxy*>(sceneProxy);
                        //FVoxelVertexFactory* VF = VoxelSceneProxy->GetVertexFactory();

                        const FScene* Scene = View.Family->Scene->GetRenderScene();
                        if (!Scene) return;

                        VoxelSceneProxy->RenderMyCustomPass(RHICmdList, Scene, &View);
                    }
                }
            }
        });

    return output;
}

void FVoxelSceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
   /* checkSlow(View.bIsViewInfo);
    const FViewInfo& viewInfo = static_cast<const FViewInfo&>(View);
    const FIntRect PrimaryViewRect = viewInfo.ViewRect;

    FScreenPassTexture sceneColor((*Inputs.SceneTextures)->SceneColorTexture, PrimaryViewRect);
    check(sceneColor.IsValid());
    FScreenPassRenderTarget SceneColorRenderTarget(sceneColor, ERenderTargetLoadAction::ELoad);
    FRHIBlendState* DefaultBlendState = FScreenPassPipelineState::FDefaultBlendState::GetRHI();*/

    // FScreenPassRenderTarget output = FScreenPassRenderTarget::CreateFromInput(GraphBuilder, sceneColor, View.GetOverwriteLoadAction(), TEXT("VoxelOutput"));
    //FTextureRHIRef RenderTargetTexture = InView.Family->RenderTarget->GetRenderTargetTexture();

    // Assorted info that is not used but may be useful********************************************************
    //FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
    //FRDGTextureDesc ColorCorrectRegionsOutputDesc = sceneColor.Texture->Desc;

   // ColorCorrectRegionsOutputDesc.Format = PF_FloatRGBA;
    //FLinearColor ClearColor(0., 0., 0., 0.);
   // ColorCorrectRegionsOutputDesc.ClearValue = FClearValueBinding(ClearColor);

   // FRDGTexture* BackBufferRenderTargetTexture = GraphBuilder.CreateTexture(ColorCorrectRegionsOutputDesc, TEXT("BackBufferRenderTargetTexture"));
    //FScreenPassRenderTarget BackBufferRenderTarget = FScreenPassRenderTarget(BackBufferRenderTargetTexture, sceneColor.ViewRect, ERenderTargetLoadAction::EClear);
    //const FScreenPassTextureViewport SceneColorTextureViewport(sceneColor);

    //RDG_EVENT_SCOPE(GraphBuilder, "Color Correct Regions %dx%d", SceneColorTextureViewport.Rect.Width(), SceneColorTextureViewport.Rect.Height());

    //FRHISamplerState* PointClampSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
   // const FScreenPassTextureViewportParameters SceneTextureViewportParams = GetTextureViewportParameters(SceneColorTextureViewport);
    /*FScreenPassTextureInput SceneTextureInput;
    {
        SceneTextureInput.Viewport = SceneTextureViewportParams;
        SceneTextureInput.Texture = SceneColorRenderTarget.Texture;
        SceneTextureInput.Sampler = PointClampSampler;
    }*/
    // end assorted info region********************************************************************************

    /*FVoxelTargetParameters* PassParameters = GraphBuilder.AllocParameters<FVoxelTargetParameters>();
    PassParameters->RenderTargets[0] = SceneColorRenderTarget.GetRenderTargetBinding();
    const FScreenPassTextureViewport RegionViewport(SceneColorRenderTarget.Texture, PrimaryViewRect);

    GraphBuilder.AddPass(
        RDG_EVENT_NAME("VoxelRenderPass"),
        PassParameters,
        ERDGPassFlags::Raster,
        [this, 
        &View, 
        DefaultBlendState,
        RegionViewport
        ](FRHICommandListImmediate& RHICmdList)
        {      

            FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
            TShaderMapRef<FVoxelVertexShader> VertexShader(GlobalShaderMap);
            TShaderMapRef<FVoxelPixelShader> PixelShader(GlobalShaderMap);
            FVoxelVertexShader::FParameters VSParams;
            FVoxelPixelShader::FParameters PSParams;*/

            /*DrawScreenPass(
                RHICmdList,
                View,
                RegionViewport,
                RegionViewport,
                FScreenPassPipelineState(VertexShader, PixelShader, DefaultBlendState),
                EScreenPassDrawFlags::None,
                [&](FRHICommandList&)
                {
                    SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VSParams);
                    SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PSParams);
                });*/

            //UE_LOG(LogTemp, Warning, TEXT("Start pass"));
            //FRHIRenderPassInfo RPInfo(output.Texture->GetRHI(), ERenderTargetActions::Load_Store);

            /*RHICmdList.SetViewport(
                PrimaryViewRect.Min.X, PrimaryViewRect.Min.Y, 0.0f,
                PrimaryViewRect.Max.X, PrimaryViewRect.Max.Y, 1.0f
            );*/

            //RHICmdList.BeginRenderPass(RPInfo, TEXT("VoxelRenderPass"));

#if false
            for (const auto& Pair : VoxelBodiesInfos)
            {
                const FVoxelBodyInfo& Info = Pair.Value;
                for (const auto& MeshComp : Info.RenderContext.VoxelBodies)
                {
                    if (MeshComp.IsValid() && MeshComp->SceneProxy)
                    {
                        FPrimitiveSceneProxy* sceneProxy = MeshComp->SceneProxy;
                        if (sceneProxy == nullptr) continue;
                        FVoxelSceneProxy* VoxelSceneProxy = static_cast<FVoxelSceneProxy*>(sceneProxy);
                        FVoxelVertexFactory* VF = VoxelSceneProxy->GetVertexFactory();

                        const FScene* Scene = View.Family->Scene->GetRenderScene();
                        if (!Scene) return;

                        VoxelSceneProxy->RenderMyCustomPass(RHICmdList, Scene, &View);
                    }
                }  
            }
#endif
            //RHICmdList.EndRenderPass();
       // });
//----------------- Attempt 4 using Global shader DrawIndexedPrimitive to render data as post process
    // ended with Shader Compliation failures are fatal. 
    // It seems like FGlobalShaders do not support vertex information so need to change to FVoxelPixelMeshMaterialShader instead
#if false
                const FMatrix ViewMatrix = View.ViewMatrices.GetViewMatrix();
                const FMatrix ProjectionMatrix = View.ViewMatrices.GetProjectionMatrix();
                const FMatrix ViewProj = ViewMatrix * ProjectionMatrix;
                const FMatrix ltw = sceneProxy->GetLocalToWorld();
                const FMatrix MVP =  ViewProj * ltw;


                FTextureRHIRef RenderTargetTexture = View.Family->RenderTarget->GetRenderTargetTexture();
                FRHICommandListImmediate& RHICmdList = GraphBuilder.RHICmdList;
                FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::Load_Store);
                RHICmdList.BeginRenderPass(RPInfo, TEXT("CustomPass"));
                
                FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
                TShaderMapRef<FVoxelVertexShader> VertexShader(GlobalShaderMap);
                TShaderMapRef<FVoxelPixelShader> PixelShader(GlobalShaderMap);
                FVoxelVertexShader::FParameters VSParams;
                FVoxelPixelShader::FParameters PSParams;

                SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VSParams);
                SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PSParams);

                FGraphicsPipelineStateInitializer GraphicsPSOInit;
                RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
                GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
                GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                GraphicsPSOInit.PrimitiveType = PT_TriangleList;

                GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VF->GetDeclaration(EVertexInputStreamType::PositionAndNormalOnly);
                GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

                uint32 count = VF->GetVertexBuffer()->GetVertexCount();

                RHICmdList.SetStreamSource(0, VF->GetVertexBuffer()->VertexBufferRHI, 0);
                RHICmdList.SetStreamSource(0, VF->GetNormalBuffer()->VertexBufferRHI, 1);

                RHICmdList.DrawIndexedPrimitive(
                    VF->GetIndexBufferRHIRef(),
                    0,
                    0,
                    count,
                    0,
                    count / 3,
                    1
                );

                RHICmdList.EndRenderPass();


#endif
}
