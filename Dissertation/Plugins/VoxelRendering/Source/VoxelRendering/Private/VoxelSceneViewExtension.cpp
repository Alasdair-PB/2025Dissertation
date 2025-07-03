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

IMPLEMENT_GLOBAL_SHADER(FVoxelPixelShader, "/VoxelShaders/VoxelPixelShader.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FVoxelVertexShader, "/VoxelShaders/VoxelVertexShader.usf", "MainVS", SF_Vertex);

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
            FVoxelBodyInfo& VoxelBodyInfo = VoxelBodiesInfos.FindOrAdd(VoxelBody); // FVoxelBodyInfo& VoxelBodyInfo = VoxelBodiesInfos.FindChecked(VoxelBody);
            VoxelBodyInfo.RenderContext.RenderedBody = VoxelBody;
            VoxelBodyInfo.RenderContext.VoxelBodies.Reset();
            UVoxelMeshComponent* MeshComp = VoxelBody->GetComponentByClass<UVoxelMeshComponent>();

            if (MeshComp && MeshComp->IsRegistered())
                VoxelBodyInfo.RenderContext.VoxelBodies.Add(MeshComp);
          
            int32_t ExpectedNumViews = 4;

            if (VoxelBodyInfo.ViewInfos.Num() != ExpectedNumViews)
                VoxelBodyInfo.ViewInfos.SetNum(ExpectedNumViews);

            //UpdateVoxelInfoRendering_CustomRenderPass(Scene, InViewFamily, &VoxelBodyInfo);
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

void FVoxelSceneViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& View)
{
    /*for (const auto& Pair : VoxelBodiesInfos)
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

                GraphBuilder.AddPass(
                    RDG_EVENT_NAME("VoxelRenderPass"),
                    ERDGPassFlags::None,
                    [this, &View, VoxelSceneProxy](FRHICommandListImmediate& RHICmdList)
                    {
                        const FScene* Scene = View.Family->Scene->GetRenderScene();
                        FTextureRHIRef RenderTargetTexture = View.Family->RenderTarget->GetRenderTargetTexture();

                        if (!Scene) return;
                        if (!RenderTargetTexture) return;

                        VoxelSceneProxy->RenderMyCustomPass(RHICmdList, Scene, &View);
                    });
            }
        }
    }
    GraphBuilder.Execute();*/
}

BEGIN_SHADER_PARAMETER_STRUCT(FVoxelTargetParameters, )
    RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

void FVoxelSceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass PassId, const FSceneView& InView, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
    //if (!CustomRenderTargetPerView_RenderThread.Contains(InView.GetViewKey()))
    //    return;

    //if (PassId == EPostProcessingPass::SSRInput)// && bCompositeSupportsSSR.load())
     //   InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FVoxelSceneViewExtension::PostProcessPassSSRInput_RenderThread));
}

FScreenPassTexture FVoxelSceneViewExtension::PostProcessPassSSRInput_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& Inputs) 
{                

    FVoxelTargetParameters* PassParameters = GraphBuilder.AllocParameters<FVoxelTargetParameters>();
                
    FScreenPassTexture sceneColor = FScreenPassTexture::CopyFromSlice(GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor));
    check(sceneColor.IsValid());
    FScreenPassRenderTarget output = FScreenPassRenderTarget::CreateFromInput(GraphBuilder, sceneColor, InView.GetOverwriteLoadAction(), TEXT("VoxelOutput"));

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

                // FRDGTextureRef Texture = GraphBuilder.RegisterExternalTexture(LightmapTilePoolGPU.PooledRenderTargets[PoolLayerIndex]);
                 //PassParameters->RenderTargets[0] = FRenderTargetBinding(Texture, ERenderTargetLoadAction::ENoAction);
                PassParameters->RenderTargets[0] = output.GetRenderTargetBinding();

                GraphBuilder.AddPass(
                    RDG_EVENT_NAME("VoxelRenderPass"),
                    PassParameters,
                    ERDGPassFlags::Raster,
                    [this, &InView, VoxelSceneProxy, output](FRHICommandListImmediate& RHICmdList)
                    {
                        const FScene* Scene = InView.Family->Scene->GetRenderScene();
                        //FTextureRHIRef RenderTargetTexture = InView.Family->RenderTarget->GetRenderTargetTexture();

                        if (!Scene) return;
                        //if (!RenderTargetTexture) return;

                        VoxelSceneProxy->RenderMyCustomPass(RHICmdList, Scene, &InView);
                    });
            }
        }
    }

    return output;
}

void FVoxelSceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
    FSceneViewExtensionBase::PrePostProcessPass_RenderThread(GraphBuilder, View, Inputs);
    FVoxelTargetParameters* PassParameters = GraphBuilder.AllocParameters<FVoxelTargetParameters>();

    checkSlow(View.bIsViewInfo);
    const FViewInfo& viewInfo = static_cast<const FViewInfo&>(View);
    const FIntRect PrimaryViewRect = viewInfo.ViewRect;

    FScreenPassTexture sceneColor((*Inputs.SceneTextures)->SceneColorTexture, PrimaryViewRect);
    check(sceneColor.IsValid());

    //FRDGTexture* BackBufferRenderTargetTexture = GraphBuilder.CreateTexture(ColorCorrectOutputDesc, TEXT("BackBufferRenderTargetTexture"));
    //FScreenPassRenderTarget output = FScreenPassRenderTarget(BackBufferRenderTargetTexture, sceneColor.ViewRect, ERenderTragetLoadAction::ELoad);

    FScreenPassRenderTarget output = FScreenPassRenderTarget::CreateFromInput(GraphBuilder, sceneColor, View.GetOverwriteLoadAction(), TEXT("VoxelOutput"));
     //FTextureRHIRef RenderTargetTexture = View.Family->RenderTarget->GetRenderTargetTexture();

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
                PassParameters->RenderTargets[0] = output.GetRenderTargetBinding();

                GraphBuilder.AddPass(
                    RDG_EVENT_NAME("VoxelRenderPass"),
                    PassParameters,
                    ERDGPassFlags::Raster,
                    [this, &View, VoxelSceneProxy, output](FRHICommandListImmediate& RHICmdList)
                    {
                        const FScene* Scene = View.Family->Scene->GetRenderScene();
                        if (!Scene) return;
                        VoxelSceneProxy->RenderMyCustomPass(RHICmdList, Scene, &View);
                    });
            }
        }
    }

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
