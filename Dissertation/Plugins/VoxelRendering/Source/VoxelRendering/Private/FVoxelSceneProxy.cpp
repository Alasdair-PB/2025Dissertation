#include "FVoxelSceneProxy.h"
#include "VoxelRendering.h"
#include "VoxelMeshPassProcessor.h"
#include "Octree.h"
#include "Materials/MaterialInterface.h"
#include "RHICommandList.h"
#include "RHIResources.h"
#include "MeshBatch.h"
#include "UniformBuffer.h"
#include "SceneView.h"
#include "RenderResource.h"

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
	FRHIRenderPassInfo RPInfo(Target, ERenderTargetActions::Load_Store);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("MyCustomPass"));

#if false
	FMeshPassDrawListContext* DrawListContext = nullptr; // abstract
	FVoxelMeshPassProcessor PassProcessor(InScene, View, DrawListContext);

	for (const FMeshBatch& MeshBatch : CustomPassMeshBatches)
		PassProcessor.AddMeshBatch(MeshBatch, ~0ull, nullptr);
#endif

	RHICmdList.EndRenderPass();
}

FORCENOINLINE void FVoxelSceneProxy::GetDynamicMeshElements(
	const TArray<const FSceneView*>& Views,
	const FSceneViewFamily& ViewFamily,
	uint32 VisibilityMap,
	FMeshElementCollector& Collector) const
{
	for (int32 viewIndex = 0; viewIndex < Views.Num(); viewIndex++)
	{
		if (!(VisibilityMap & (1 << viewIndex))) continue;
		/*FMeshBatch& meshBatch = Collector.AllocateMesh();
		SetMeshBatchGeneric(meshBatch, viewIndex);
		SetMeshBatchElementsGeneric(meshBatch, viewIndex);
		Collector.AddMesh(viewIndex, meshBatch);*/
	}
}

void FVoxelSceneProxy::SetMeshBatchGeneric(FMeshBatch& meshBatch, int32 viewIndex, bool bWireframe) const {
	meshBatch.VertexFactory = VertexFactory;
	meshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
	meshBatch.Type = PT_TriangleList;
	meshBatch.DepthPriorityGroup = SDPG_World;
	meshBatch.bCanApplyViewModeOverrides = false;
	meshBatch.bWireframe = bWireframe;
	SetMeshBatchRenderProxy(meshBatch, viewIndex, bWireframe);
}

void FVoxelSceneProxy::SetMeshBatchRenderProxy(FMeshBatch& meshBatch, int32 viewIndex, bool bWireframe) const {
#if false
	FMaterialRenderProxy* renderProxy;

	if (!Material) {
		renderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
		UE_LOG(LogTemp, Warning, TEXT("No Material instance set:: switching to default MD_Surface support"));
	}
	else renderProxy = Material->GetRenderProxy();
	renderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
#else 
	meshBatch.MaterialRenderProxy = nullptr;
#endif

}

void FVoxelSceneProxy::SetMeshBatchElementsGeneric(FMeshBatch& meshBatch, int32 viewIndex) const {
	FVoxelIndexBuffer* indexBuffer = VertexFactory->GetIndexBuffer();
	uint32 numTriangles = indexBuffer->GetIndexCount() / 3;
	uint32 maxVertexIndex = VertexFactory->GetVertexBuffer()->GetVertexCount() - 1;

	FMeshBatchElement& batchElement = meshBatch.Elements[0];
	batchElement.IndexBuffer = indexBuffer;
	batchElement.FirstIndex = 0;
	batchElement.NumPrimitives = numTriangles;
	batchElement.MinVertexIndex = 0;
	batchElement.MaxVertexIndex = maxVertexIndex - 1;
	batchElement.PrimitiveUniformBuffer = GetUniformBuffer();

	SetMeshBatchElementsUserData(batchElement);
}

void FVoxelSceneProxy::SetMeshBatchElementsUserData(FMeshBatchElement& batchElement) const
{
#if false
	//meshBatch.UserData = OverrideColorVertexBuffer;
	//meshBatch.bUserDataIsColorVertexBuffer = true;
#else 
	FVoxelBatchElementUserData userData;
	batchElement.UserData = &userData;
#endif
}



