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
	return bInitialized;
}

void FVoxelSceneProxy::CreateRenderThreadResources(FRHICommandListBase& RHICmdList) {
	FPrimitiveSceneProxy::CreateRenderThreadResources(RHICmdList);
	bInitialized = true;
}

void FVoxelSceneProxy::DestroyRenderThreadResources() {

}

void FVoxelSceneProxy::OnTransformChanged(FRHICommandListBase& RHICmdList) {
	FPrimitiveSceneProxy::OnTransformChanged(RHICmdList);
}

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

		for (const FVoxelProxyUpdateDataNode& node : selectedNodes)
		{
			if ((node.vertexFactory.IsValid() && node.vertexFactory->IsInitialized()))
			{
				FMeshBatch& meshBatch = Collector.AllocateMesh();
				SetMeshBatchGeneric(meshBatch, node, viewIndex);
				SetMeshBatchElementsGeneric(meshBatch, node, viewIndex);

				//CustomPassMeshBatches.Add(FMeshBatch(meshBatch));
				Collector.AddMesh(viewIndex, meshBatch);
			}
		}
	}
}

void FVoxelSceneProxy::UpdateSelectedNodes(const TArray<FVoxelProxyUpdateDataNode>& renderData) {
	selectedNodes = renderData;
}

void FVoxelSceneProxy::SetMeshBatchGeneric(FMeshBatch& meshBatch, const FVoxelProxyUpdateDataNode& node, int32 viewIndex, bool bWireframe) const {
	check(node.vertexFactory);
	meshBatch.VertexFactory = node.vertexFactory.Get();
	meshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
	meshBatch.Type = PT_TriangleList;
	meshBatch.DepthPriorityGroup = SDPG_World;
	meshBatch.bCanApplyViewModeOverrides = false;
	meshBatch.bWireframe = bWireframe;
	SetMeshBatchRenderProxy(meshBatch);
}

void FVoxelSceneProxy::SetMeshBatchRenderProxy(FMeshBatch& meshBatch) const {
	FMaterialRenderProxy* renderProxy;
	if (!Material) {
		renderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
		UE_LOG(LogTemp, Warning, TEXT("No Material instance set:: switching to default MD_Surface support"));
	}
	else{
		renderProxy = Material->GetRenderProxy();
	}		
	check(renderProxy);
	meshBatch.MaterialRenderProxy = renderProxy;
}

void FVoxelSceneProxy::SetMeshBatchElementsGeneric(FMeshBatch& meshBatch, const FVoxelProxyUpdateDataNode& node, int32 viewIndex) const {
	FVoxelIndexBuffer* indexBuffer = node.vertexFactory->GetIndexBuffer();
	uint32 numTriangles = (indexBuffer->GetVisibleIndiceCount()) / 3;
	uint32 maxVertexIndex = node.vertexFactory->GetVertexBuffer()->GetVisibleVerticiesCount() - 1;

	check(indexBuffer);
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
	FVoxelBatchElementUserData userData;
	batchElement.UserData = nullptr;
}



