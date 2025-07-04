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
	Material->ForceRecompileForRendering();

	uint32 size = 64 * nodeVoxelCount * 15; // instead of 64 should be * Params.Input.leafCount;

	size += 1000;
	VertexFactory = new FVoxelVertexFactory(GetScene().GetFeatureLevel(), size);
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
		Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
	}
	return Result;
}

bool FVoxelSceneProxy::IsInitialized() {
	return bInitialized && VertexFactory == nullptr ? false : VertexFactory->bInitialized;
}

FVoxelVertexFactory* FVoxelSceneProxy::GetVertexFactory() { return VertexFactory; }

void FVoxelSceneProxy::CreateRenderThreadResources(FRHICommandListBase& RHICmdList) {
	VertexFactory->InitResource(RHICmdList);
	FPrimitiveSceneProxy::CreateRenderThreadResources(RHICmdList);
	bInitialized = true;
}

void FVoxelSceneProxy::DestroyRenderThreadResources() {
	if (VertexFactory != nullptr)
	{
		VertexFactory->ReleaseResource();
		//delete VertexFactory;
		VertexFactory = nullptr;
	}
}

void FVoxelSceneProxy::OnTransformChanged(FRHICommandListBase& RHICmdList) {
	FPrimitiveSceneProxy::OnTransformChanged(RHICmdList);
}

//FMeshBatch MeshBatch;
//SetMeshBatchGeneric(MeshBatch, View->PrimaryViewIndex);
//SetMeshBatchElementsGeneric(MeshBatch, View->PrimaryViewIndex);
//MeshProcessor.AddMeshBatch(MeshBatch, ~0ull, this);

void FVoxelSceneProxy::RenderMyCustomPass(FRHICommandListImmediate& RHICmdList, const FScene* InScene, const FSceneView* View)
{
	check(InScene);
	check(View);
	if (!IsInitialized()) return;

	FDynamicMeshDrawCommandStorage DynamicMeshDrawCommandStorage;
	FMeshCommandOneFrameArray VisibleMeshDrawCommands;
	FGraphicsMinimalPipelineStateSet GraphicsMinimalPipelineStateSet;
	bool NeedsShaderInitialisation = false;

	TArray<FMeshBatch> MeshBatchesToDrawImmediately;
	TArray<FMeshBatch> MeshBatchesNeedingInstanceOffsetUpdates;
	for (auto& MeshBatch : CustomPassMeshBatches)
	{
		FMeshBatchElement& Element = MeshBatch.Elements[0];
		if (Element.UserIndex == INDEX_NONE) MeshBatchesToDrawImmediately.Add(MeshBatch);
		else MeshBatchesNeedingInstanceOffsetUpdates.Add(MeshBatch);
	}

	DrawDynamicMeshPass(
		*View, RHICmdList,
		[this, &View, InScene, MeshBatchesToDrawImmediately](FDynamicPassMeshDrawListContext* DynamicMeshPassContext)
		{
			FVoxelMeshPassProcessor MeshProcessor(InScene, View, DynamicMeshPassContext);
			for (auto& MeshBatch : MeshBatchesToDrawImmediately)
				MeshProcessor.AddMeshBatch(MeshBatch, ~0ull, this);
		});

	for (auto& MeshBatch : MeshBatchesNeedingInstanceOffsetUpdates)
	{	
		FDynamicPassMeshDrawListContext DynamicMeshPassContext(DynamicMeshDrawCommandStorage, VisibleMeshDrawCommands, GraphicsMinimalPipelineStateSet, NeedsShaderInitialisation);
		FVoxelMeshPassProcessor MeshProcessor(InScene, View, &DynamicMeshPassContext);
		MeshProcessor.AddMeshBatch(MeshBatch, ~0ull, this);

		const uint32 InstanceFactor = 1;
		FRHIBuffer* PrimitiveIdVertexBuffer = nullptr;
		const bool bDynamicInstancing = false;
		const uint32 PrimitiveIdBufferStride = FInstanceCullingContext::GetInstanceIdBufferStride(View->GetShaderPlatform());

		for (FVisibleMeshDrawCommand& Cmd : VisibleMeshDrawCommands)
			Cmd.PrimitiveIdInfo.DrawPrimitiveId = MeshBatch.Elements[0].UserIndex;

		SortAndMergeDynamicPassMeshDrawCommands(*View, RHICmdList, VisibleMeshDrawCommands, DynamicMeshDrawCommandStorage, PrimitiveIdVertexBuffer, InstanceFactor, nullptr);
		FMeshDrawCommandSceneArgs SceneArgs;
		SceneArgs.PrimitiveIdsBuffer = PrimitiveIdVertexBuffer;
		SubmitMeshDrawCommands(VisibleMeshDrawCommands, GraphicsMinimalPipelineStateSet, SceneArgs, PrimitiveIdBufferStride, bDynamicInstancing, InstanceFactor, RHICmdList);
	}

	CustomPassMeshBatches.Empty();
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
		FMeshBatch& meshBatch = Collector.AllocateMesh();

		SetMeshBatchGeneric(meshBatch, viewIndex);
		SetMeshBatchElementsGeneric(meshBatch, viewIndex);

		CustomPassMeshBatches.Add(FMeshBatch(meshBatch));
		Collector.AddMesh(viewIndex, meshBatch);
	}
}

void FVoxelSceneProxy::SetMeshBatchGeneric(FMeshBatch& meshBatch, int32 viewIndex, bool bWireframe) const {
	check(VertexFactory);
	meshBatch.VertexFactory = VertexFactory;
	meshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
	meshBatch.Type = PT_TriangleList;
	meshBatch.DepthPriorityGroup = SDPG_World;
	meshBatch.bCanApplyViewModeOverrides = false;
	meshBatch.bWireframe = bWireframe;
	SetMeshBatchRenderProxy(meshBatch, viewIndex, bWireframe);
}

void FVoxelSceneProxy::SetMeshBatchRenderProxy(FMeshBatch& meshBatch, int32 viewIndex, bool bWireframe) const {
	FMaterialRenderProxy* renderProxy;
	/*if (!Material) {
		renderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
		UE_LOG(LogTemp, Warning, TEXT("No Material instance set:: switching to default MD_Surface support"));
	}
	else*/ 
	renderProxy = Material->GetRenderProxy();

	meshBatch.MaterialRenderProxy = renderProxy;
}

void FVoxelSceneProxy::SetMeshBatchElementsGeneric(FMeshBatch& meshBatch, int32 viewIndex) const {
	FVoxelIndexBuffer* indexBuffer = VertexFactory->GetIndexBuffer();
	uint32 numTriangles = (indexBuffer->GetIndexCount() - 1000) / 3;
	uint32 maxVertexIndex = VertexFactory->GetVertexBuffer()->GetVertexCount() - 1 - 1000;

	FMeshBatchElement& batchElement = meshBatch.Elements[0];
	batchElement.IndexBuffer = indexBuffer;
	batchElement.FirstIndex = 0;
	batchElement.NumPrimitives = numTriangles;
	batchElement.MinVertexIndex = 0;
	batchElement.MaxVertexIndex = maxVertexIndex;
	batchElement.PrimitiveUniformBuffer = GetUniformBuffer();

	SetMeshBatchElementsUserData(batchElement);
}

void FVoxelSceneProxy::SetMeshBatchElementsUserData(FMeshBatchElement& batchElement) const
{
#if false
	FVoxelBatchElementUserData userData;
	batchElement.UserData = userData;
#else 
	batchElement.UserData = nullptr;
#endif
}



