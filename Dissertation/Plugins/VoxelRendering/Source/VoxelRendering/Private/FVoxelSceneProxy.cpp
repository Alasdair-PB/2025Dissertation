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
#include "Async/Mutex.h"

FPrimitiveViewRelevance FVoxelSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = true;
	Result.bDynamicRelevance = true;
	Result.bOpaqueRelevance = true;
	return Result;
}

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

	FVoxelVertexFactoryParameters UniformParams;

	VertexFactory = new FVoxelVertexFactory(GetScene().GetFeatureLevel());
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

void FVoxelSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI, int LODIndex) {
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
	Mesh.LODIndex = LODIndex;
	Mesh.bDitheredLODTransition = false;

	FMeshBatchElement& meshBatch = Mesh.Elements[0];
	meshBatch.IndexBuffer = VertexFactory->GetIndexBuffer();
	meshBatch.FirstIndex = 0;
	meshBatch.NumPrimitives = 0; // IndexBuffer.GetIndexDataSize() / 3;
	meshBatch.MinVertexIndex = 0;
	meshBatch.MaxVertexIndex = 0; // VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;

	PDI->DrawMesh(Mesh, FLT_MAX);
}


FORCEINLINE void FVoxelSceneProxy::DrawDynamicElements(FMeshBatch& Mesh, FMaterialRenderProxy* MaterialProxy, bool bWireframe, int32 ViewIndex) const {
	//if (VertexBuffers.PositionVertexBuffer.GetNumVertices() == 0) return;
	check(MaterialProxy);

	bool bHasPrecomputedVolumetricLightmap;
	FMatrix PreviousLocalToWorld;
	int32 SingleCaptureIndex;
	bool bOutputVelocity;

	GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);

	FMeshBatchElement& BatchElement = Mesh.Elements[0];
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = 0; // IndexBuffer.GetIndexDataSize() / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = 0; // VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
	BatchElement.IndexBuffer = VertexFactory->GetIndexBuffer();

	Mesh.bWireframe = bWireframe;
	Mesh.VertexFactory = VertexFactory;
	Mesh.MaterialRenderProxy = MaterialProxy;
	Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
	Mesh.Type = PT_TriangleList;
	Mesh.DepthPriorityGroup = SDPG_World;
	Mesh.bCanApplyViewModeOverrides = false;
}
