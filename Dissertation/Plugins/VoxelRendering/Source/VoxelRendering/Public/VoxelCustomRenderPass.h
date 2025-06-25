#pragma once
#include "CoreMinimal.h"
#include "FVoxelSceneProxy.h"
#include "SceneView.h"
#include "Rendering/CustomRenderPass.h"

struct FMyVoxelRenderData : public ICustomRenderPassUserData
{
    IMPLEMENT_CUSTOM_RENDER_PASS_USER_DATA(FMyVoxelRenderData);

    FMyVoxelRenderData(FVoxelSceneProxy* InVoxelProxy)
        : VoxelProxy(InVoxelProxy) {}

    FVoxelSceneProxy* GetVoxelProxy() const { return VoxelProxy; }

private:
    FVoxelSceneProxy* VoxelProxy = nullptr;
};

class VOXELRENDERING_API FVoxelCustomRenderPass : public FCustomRenderPassBase
{
public:
    FVoxelCustomRenderPass(const FIntPoint& InRenderTargetSize) :
        FCustomRenderPassBase(TEXT("VoxelInfoPass"), FCustomRenderPassBase::ERenderMode::DepthAndBasePass, FCustomRenderPassBase::ERenderOutput::SceneColorAndDepth, InRenderTargetSize)
    {}

    virtual void OnPreRender(FRDGBuilder& GraphBuilder) override
    {
        FCustomRenderPassBase::OnPreRender(GraphBuilder);

        if (Views.Num() == 0) return;
        const FScene* Scene = Views[0]->Family->Scene->GetRenderScene();
        FTextureRHIRef RenderTargetTexture = View[0]->Family->RenderTarget->GetRenderTargetTexture();

        if (!Scene || !RenderTargetTexture) return;

        FMyVoxelRenderData* VoxelData = static_cast<FMyVoxelRenderData*>(UserDatas.FindRef("FMyVoxelRenderData").Get());
        FVoxelSceneProxy* sceneProxy = VoxelData->GetVoxelProxy();
        const FSceneView* sceneView = ;

        if (VoxelData && sceneProxy)
        {
            GraphBuilder.AddPass(
                RDG_EVENT_NAME("VoxelRenderPass"),
                ERDGPassFlags::None,
                [this, Scene, RenderTargetTexture, sceneView, sceneProxy](FRHICommandListImmediate& RHICmdList)
                {
                    sceneProxy->RenderMyCustomPass(RHICmdList, Scene, sceneView);
                });
        }

    }

    virtual void OnPostRender(FRDGBuilder& GraphBuilder) override
    {
        FCustomRenderPassBase::OnPostRender(GraphBuilder);
    }

	virtual void OnBeginPass(FRDGBuilder& GraphBuilder) override {

	}

	virtual void OnEndPass(FRDGBuilder& GraphBuilder) override {}
};
