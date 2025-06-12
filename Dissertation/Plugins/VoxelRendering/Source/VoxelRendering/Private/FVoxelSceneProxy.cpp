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
#include "Async/Mutex.h"


FVoxelSceneProxy::FVoxelSceneProxy(UPrimitiveComponent* Component) :
	FPrimitiveSceneProxy(Component),
	bCompatiblePlatform(GetScene().GetFeatureLevel() >= ERHIFeatureLevel::SM5)
{

}

FVoxelSceneProxy::~FVoxelSceneProxy() {
	// Safe release any buffers in this SceneProxy
}

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

FVoxelVertexFactory* FVoxelSceneProxy::GetVertexFactor() { return VertexFactory; }

FORCENOINLINE void FVoxelSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	FMaterialRenderProxy* renderProxy = Material->GetRenderProxy();
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (!(VisibilityMap & (1 << ViewIndex))) continue;
		FMeshBatch& Mesh = Collector.AllocateMesh();
		DrawDynamicElements(Mesh, renderProxy, false, 0);
		Collector.AddMesh(ViewIndex, Mesh);
	}
}

void FVoxelSceneProxy::CreateRenderThreadResources(FRHICommandListBase& RHICmdList) {

	FVoxelVertexFactoryUniformParameters UniformParams;
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

/*void FVoxelSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) {
	if (RuntimeVirtualTextureMaterialTypes.Num() == 0) return;

	FMaterialRenderProxy* MaterialInstance = Material->GetRenderProxy();
	FMeshBatch Mesh;

	Mesh.bWireframe = false;
	Mesh.VertexFactory = VertexFactory;
	Mesh.MaterialRenderProxy = MaterialInstance;
	Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
	Mesh.Type = PT_TriangleList;
	Mesh.DepthPriorityGroup = SDPG_World;
	Mesh.bCanApplyViewModeOverrides = false;
	// Mesh.LODIndex = LODIndex;
	Mesh.bDitheredLODTransition = false;

	FMeshBatchElement& meshBatch = Mesh.Elements[0];
	FVoxelIndexBuffer* indexBuffer = VertexFactory->GetIndexBuffer();

	meshBatch.IndexBuffer = indexBuffer;
	meshBatch.FirstIndex = 0;
	meshBatch.NumPrimitives = indexBuffer->GetIndexCount() / 3;
	meshBatch.MinVertexIndex = 0;
	meshBatch.MaxVertexIndex = VertexFactory->GetVertexBuffer()->GetVertexCount() - 1;
	PDI->DrawMesh(Mesh, FLT_MAX);
}*/


FORCEINLINE void FVoxelSceneProxy::DrawDynamicElements(FMeshBatch& Mesh, FMaterialRenderProxy* MaterialProxy, bool bWireframe, int32 ViewIndex) const {
	check(MaterialProxy);

	bool bHasPrecomputedVolumetricLightmap;
	FMatrix PreviousLocalToWorld;
	int32 SingleCaptureIndex;
	bool bOutputVelocity;

	GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);

	FMeshBatchElement& meshBatch = Mesh.Elements[0];
	FVoxelIndexBuffer* indexBuffer = VertexFactory->GetIndexBuffer();

	meshBatch.FirstIndex = 0;
	meshBatch.NumPrimitives = indexBuffer->GetIndexCount() / 3;
	meshBatch.MinVertexIndex = 0;
	meshBatch.MaxVertexIndex = VertexFactory->GetVertexBuffer()->GetVertexCount() - 1;
	meshBatch.IndexBuffer = indexBuffer;

	Mesh.bWireframe = bWireframe;
	Mesh.VertexFactory = VertexFactory;
	Mesh.MaterialRenderProxy = MaterialProxy;
	Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
	Mesh.Type = PT_TriangleList;
	Mesh.DepthPriorityGroup = SDPG_World;
	Mesh.bCanApplyViewModeOverrides = false;
}
