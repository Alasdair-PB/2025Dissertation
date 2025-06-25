#pragma once
#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "AVoxelBody.h"
#include "Misc/ScopeLock.h"
#include "FVoxelSceneProxy.h"

class UTextureRenderTarget2DArray;

template <typename KeyType, typename ValueType>
using TWeakObjectPtrKeyMap = TMap<TWeakObjectPtr<KeyType>, ValueType, FDefaultSetAllocator, TWeakObjectPtrMapKeyFuncs<TWeakObjectPtr<KeyType>, ValueType>>;

class VOXELRENDERING_API FVoxelSceneViewExtension : public FWorldSceneViewExtension
{
public:	
	struct FRenderingContext
	{
		AVoxelBody* RenderedBody = nullptr;
		//UTextureRenderTarget2DArray* TextureRenderTarget;
		TArray<TWeakObjectPtr<UVoxelMeshComponent>> VoxelBodies;
		//float CaptureZ;
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
		// 1 per viewPlayer
		TArray<FVoxelBodyViewInfo, TInlineAllocator<4>> ViewInfos;
	};
	TWeakObjectPtrKeyMap<AVoxelBody, FVoxelBodyInfo> VoxelBodiesInfos;


	FVoxelSceneViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld)
		: FWorldSceneViewExtension(AutoReg, InWorld) {}

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {};
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {};

	virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override {};
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {};
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {};
	virtual void PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {};
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override;
	virtual void PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {};

	void UpdateVoxelInfoRendering_CustomRenderPass(FSceneInterface* Scene, const FSceneViewFamily& ViewFamily, const FVoxelBodyInfo* VoxelBodyInfo);
};