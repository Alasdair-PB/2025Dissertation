#pragma once
#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "AVoxelBody.h"
#include "FVoxelSceneProxy.h"



//class UTextureRenderTarget2DArray;

template <typename KeyType, typename ValueType>
using TWeakObjectPtrKeyMap = TMap<TWeakObjectPtr<KeyType>, ValueType, FDefaultSetAllocator, TWeakObjectPtrMapKeyFuncs<TWeakObjectPtr<KeyType>, ValueType>>;

class VOXELRENDERING_API FVoxelSceneViewExtension : public FWorldSceneViewExtension
{
public:	

#if true
	//----------------- Attempt 1 data: see cpp file- may be refactored to be included.

	struct FRenderingContext
	{
		AVoxelBody* RenderedBody = nullptr;
		//UTextureRenderTarget2DArray* TextureRenderTarget;
		TArray<TWeakObjectPtr<UVoxelMeshComponent>> VoxelBodies;
	};

	struct FVoxelBodyInfo
	{
		FRenderingContext RenderContext;
		struct FVoxelBodyViewInfo
		{
			//TOptional<FBox2D> UpdateBounds = FBox2D(ForceInit);
			FVector Center = FVector(ForceInit);
			FVoxelSceneProxy* OldSceneProxy = nullptr;
			bool bIsDirty = true;
		};
		TArray<FVoxelBodyViewInfo, TInlineAllocator<4>> ViewInfos;
	};
	TWeakObjectPtrKeyMap<AVoxelBody, FVoxelBodyInfo> VoxelBodiesInfos;	
	
	void UpdateVoxelInfoRendering_CustomRenderPass(FSceneInterface* Scene, const FSceneViewFamily& ViewFamily, const FVoxelBodyInfo* VoxelBodyInfo);
#endif

	FVoxelSceneViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld)
		: FWorldSceneViewExtension(AutoReg, InWorld) {}

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {};
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {};

	virtual void SubscribeToPostProcessingPass(EPostProcessingPass PassId, const FSceneView& InView, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;
	FScreenPassTexture PostProcessPassSSRInput_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& Inputs);

	virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override {};
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {};
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;
	virtual void PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {};
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override;
	virtual void PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {};

};