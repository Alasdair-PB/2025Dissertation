#include "FVoxelSceneProxy.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "EngineModule.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "GlobalRenderResources.h"
#include "GlobalShader.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "PrimitiveViewRelevance.h"
#include "RHIStaticStates.h"
#include "RenderGraphUtils.h"
#include "SceneInterface.h"
#include "TextureResource.h"
#include "Octree.h"
#include "VoxelPixelShader.h"
#include "VoxelVertexShader.h"
#include "Async/Mutex.h"
#include "VoxelMeshPassProcessor.h"

FVoxelSceneProxy::FVoxelSceneProxy(UPrimitiveComponent* Component) :
	FPrimitiveSceneProxy(Component),
	bCompatiblePlatform(GetScene().GetFeatureLevel() >= ERHIFeatureLevel::SM5)
{
	Material = Component->GetMaterial(0);
}

FVoxelSceneProxy::~FVoxelSceneProxy() {}

FPrimitiveViewRelevance FVoxelSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;

	if (CanBeRendered())
	{
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bStaticRelevance = false;
		Result.bOpaque = true;

		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		//MaterialRelevance.SetPrimitiveViewRelevance(Result);
		Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
	}
	return Result;
}

FVoxelVertexFactory* FVoxelSceneProxy::GetVertexFactory() { return VertexFactory; }

void FVoxelSceneProxy::CreateRenderThreadResources(FRHICommandListBase& RHICmdList) {

	//FVoxelVertexFactoryUniformParameters UniformParams;
	uint32 size = 64 * nodeVoxelCount * 15; // instead of 64 should be * Params.Input.leafCount;
	VertexFactory = new FVoxelVertexFactory(GetScene().GetFeatureLevel(), size);
	VertexFactory->InitResource(RHICmdList);
	FPrimitiveSceneProxy::CreateRenderThreadResources(RHICmdList);
}

void FVoxelSceneProxy::DestroyRenderThreadResources() {
	if (VertexFactory != nullptr)
	{
		VertexFactory->ReleaseResource();
		delete VertexFactory;
		VertexFactory = nullptr;
	}
}

void FVoxelSceneProxy::OnTransformChanged(FRHICommandListBase& RHICmdList) {
	FPrimitiveSceneProxy::OnTransformChanged(RHICmdList);
}

void FVoxelSceneProxy::RenderMyCustomPass(FRHICommandListImmediate& RHICmdList, const FScene* InScene, const FSceneView* View, FTextureRHIRef Target)
{
	/*FRHIRenderPassInfo RPInfo(Target, ERenderTargetActions::Load_Store);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("MyCustomPass"));

	FMeshPassDrawListContext* DrawListContext = nullptr; // abstract
	FMyMeshPassProcessor PassProcessor(InScene, View, DrawListContext);

	for (const FMeshBatch& MeshBatch : CustomPassMeshBatches)
		PassProcessor.AddMeshBatch(MeshBatch, ~0ull, nullptr);
	RHICmdList.EndRenderPass();*/
}

FORCENOINLINE void FVoxelSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	//CustomPassMeshBatches.Reset();
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (!(VisibilityMap & (1 << ViewIndex))) continue;

		FMeshBatch& Mesh = Collector.AllocateMesh();
		FMaterialRenderProxy* renderProxy;

		if (!Material) {
			renderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
			UE_LOG(LogTemp, Warning, TEXT("No Material instance set:: switching to default MD_Surface support"));
		}
		else renderProxy = Material->GetRenderProxy();


		renderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();

		DrawDynamicElements(Mesh, Collector, renderProxy, false, ViewIndex);
		//CustomPassMeshBatches.Add(Mesh);
		Collector.AddMesh(ViewIndex, Mesh);
	}
}

FORCEINLINE void FVoxelSceneProxy::DrawDynamicElements(FMeshBatch& Mesh, FMeshElementCollector& Collector, FMaterialRenderProxy* renderProxy, bool bWireframe, int32 ViewIndex) const {
	FMeshBatchElement& meshBatch = Mesh.Elements[0];
	FVoxelIndexBuffer* indexBuffer = VertexFactory->GetIndexBuffer();

	meshBatch.FirstIndex = 0;
	meshBatch.NumPrimitives = indexBuffer->GetIndexCount() / 3;
	meshBatch.MinVertexIndex = 0;
	meshBatch.MaxVertexIndex = VertexFactory->GetVertexBuffer()->GetVertexCount() - 1;
	meshBatch.IndexBuffer = indexBuffer;
	meshBatch.PrimitiveUniformBuffer = GetUniformBuffer();

	FVoxelBatchElementUserData userData;
	meshBatch.UserData = &userData;

	Mesh.MaterialRenderProxy = renderProxy;
	Mesh.bWireframe = bWireframe;
	Mesh.VertexFactory = VertexFactory;
	Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
	Mesh.Type = PT_TriangleList;
	Mesh.DepthPriorityGroup = SDPG_World;
	Mesh.bCanApplyViewModeOverrides = false;
}

	//meshBatch.UserData = OverrideColorVertexBuffer;
	//meshBatch.bUserDataIsColorVertexBuffer = true;
/*
	FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
	DynamicPrimitiveUniformBuffer.Set(Collector.GetRHICommandList(), GetLocalToWorld(), GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, false, AlwaysHasVelocity());
	meshBatch.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;
*/

/*
FORCENOINLINE void FVoxelSceneProxy::GetDynamicMeshElements(
	const TArray<const FSceneView*>& Views,
	const FSceneViewFamily& ViewFamily,
	uint32 VisibilityMap,
	FMeshElementCollector& Collector) const
{
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (!(VisibilityMap & (1 << ViewIndex))) continue;

		const FSceneView* View = Views[ViewIndex];
		const FMatrix ViewMatrix = View->ViewMatrices.GetViewMatrix();
		const FMatrix ProjectionMatrix = View->ViewMatrices.GetProjectionMatrix();
		const FMatrix ViewProj = ViewMatrix * ProjectionMatrix;
		const FMatrix ltw = GetLocalToWorld();
		const FMatrix MVP = ltw * ViewProj;

		FVoxelVertexFactory* VF = VertexFactory;

		ENQUEUE_RENDER_COMMAND(DrawCustomShader)(
			[VertexFactory = VF, MVP](FRHICommandListImmediate& RHICmdList)
			{
				TShaderMapRef<FVoxelVertexShader> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				TShaderMapRef<FVoxelPixelShader> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

				FGraphicsPipelineStateInitializer PSOInit;

				//PSOInit.RenderTargetFormats[0] = GPixelFormats[PF_B8G8R8A8].PlatformFormat;
				//for (int32 i = 1; i < MaxSimultaneousRenderTargets; ++i)
				//	PSOInit.RenderTargetFormats[i] = PF_Unknown;

				FRHIRenderPassInfo RPInfo(TargetTexture->GetRenderTargetResource()->GetRenderTargetTexture(), ERenderTargetActions::Load_Store);
				RHICmdList.BeginRenderPass(RPInfo, TEXT("CustomPass"));
				RHICmdList.ApplyCachedRenderTargets(PSOInit);

				PSOInit.BlendState = TStaticBlendState<>::GetRHI();
				PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
				PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

				PSOInit.BoundShaderState.VertexDeclarationRHI = VertexFactory->GetDeclaration();
				PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
				PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
				PSOInit.PrimitiveType = PT_TriangleList;


				SetGraphicsPipelineState(RHICmdList, PSOInit, 0);

				FVoxelVertexShader::FParameters VSParams;
				VSParams.ModelViewProjection = FMatrix44f(MVP);
				SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VSParams);

				FVoxelPixelShader::FParameters PSParams;
				SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PSParams);

				RHICmdList.SetStreamSource(0, VertexFactory->GetVertexBuffer()->VertexBufferRHI, 0);
				RHICmdList.DrawIndexedPrimitive(
					VertexFactory->GetIndexBufferRHIRef(),
					0, // BaseVertexIndex
					0, // MinIndex
					VertexFactory->GetVertexBuffer()->GetVertexCount(), // NumVertices
					0, // StartIndex
					VertexFactory->GetIndexBuffer()->GetIndexCount() / 3, // NumPrimitives
					1  // NumInstances
				);
				RHICmdList.EndRenderPass();
			});
	}
}*/
