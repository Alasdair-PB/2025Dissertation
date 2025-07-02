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

    static bool bUpdatingVoxelInfo = false; // Prevent re-entrancy. 
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

void FVoxelSceneViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) 
{

#if false
    //----------------- Attempt 2 using mesh batch draw commands directly in View Extension 

   const FScene* InScene = InView.Family->Scene->GetRenderScene();
   if (InScene) {
        FDynamicMeshDrawCommandStorage DynamicMeshDrawCommandStorage;
        FMeshCommandOneFrameArray VisibleMeshDrawCommands;
        FGraphicsMinimalPipelineStateSet GraphicsMinimalPipelineStateSet;
        bool NeedsShaderInitialisation = false;
        FDynamicPassMeshDrawListContext DynamicMeshPassContext(DynamicMeshDrawCommandStorage, VisibleMeshDrawCommands, GraphicsMinimalPipelineStateSet, NeedsShaderInitialisation);
        FVoxelMeshPassProcessor MeshProcessor(InScene, &InView, &DynamicMeshPassContext);

        for (const FMeshBatchAndRelevance& MeshBatch : InView.)
        {
            MeshProcessor.AddMeshBatch(*MeshBatch.Mesh, MeshBatch.Mesh->, MeshBatch.PrimitiveSceneProxy);
        }
    }
#endif
}

void FVoxelSceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
#if true
	FSceneViewExtensionBase::PrePostProcessPass_RenderThread(GraphBuilder, View, Inputs);

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
        }
    }
#endif
}
