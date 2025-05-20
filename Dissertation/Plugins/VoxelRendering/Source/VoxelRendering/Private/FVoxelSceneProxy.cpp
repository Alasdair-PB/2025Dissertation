#include "FVoxelSceneProxy.h"

FPrimitiveViewRelevance FVoxelSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = true;
	Result.bDynamicRelevance = true;
	Result.bOpaqueRelevance = true;
	return Result;
}

void FVoxelSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI, FVoxelProxySection* Section, int LODIndex) {
	if (RuntimeVirtualTextureMaterialTypes.Num() == 0) return;

	FMaterialRenderProxy* MaterialInstance = Section->Material->GetRenderProxy();
	FMeshBatch Mesh;

	Mesh.bWireframe = false;
	Mesh.VertexFactory = &Section->VertexFactory;
	Mesh.MaterialRenderProxy = MaterialInstance;
	Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
	Mesh.Type = PT_TriangleList;
	Mesh.DepthPriorityGroup = SDPG_World;
	Mesh.bCanApplyViewModeOverrides = false;
	Mesh.LODIndex = LODIndex;
	Mesh.bDitheredLODTransition = false;

	FMeshBatchElement& meshBatch = Mesh.Elements[0];
	meshBatch.IndexBuffer = &Section->IndexBuffer;
	meshBatch.FirstIndex = 0;
	meshBatch.NumPrimitives = Section->IndexBuffer.GetIndexDataSize() / 3;
	meshBatch.MinVertexIndex = 0;
	meshBatch.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;

	PDI->DrawMesh(Mesh, FLT_MAX);
}


FORCEINLINE void FVoxelSceneProxy::DrawDynamicElements(const FVoxelProxySection* Section, FMeshBatch& Mesh, FMaterialRenderProxy* MaterialProxy, bool bWireframe, int32 ViewIndex) const {
	if (Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() == 0) return;

	check(MaterialProxy);
	FMeshBatchElement& BatchElement = Mesh.Elements[0];
	BatchElement.IndexBuffer = &Section->IndexBuffer;
	Mesh.bWireframe = bWireframe;
	Mesh.VertexFactory = &Section->VertexFactory;
	Mesh.MaterialRenderProxy = MaterialProxy;

	bool bHasPrecomputedVolumetricLightmap;
	FMatrix PreviousLocalToWorld;
	int32 SingleCaptureIndex;
	bool bOutputVelocity;

	GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = Section->IndexBuffer.GetIndexDataSize() / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
	//BatchElement.IndexBuffer = Section->IndexBuffer;

	Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
	Mesh.Type = PT_TriangleList;
	Mesh.DepthPriorityGroup = SDPG_World;
	Mesh.bCanApplyViewModeOverrides = false;
}

FORCENOINLINE void FVoxelSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	FVoxelProxySection* section;
	int LODIndex = 8;
	FMaterialRenderProxy* renderProxy = section->Material->GetRenderProxy();
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (!(VisibilityMap & (1 << ViewIndex))) continue;
		FMeshBatch& Mesh = Collector.AllocateMesh();
		DrawDynamicElements(section, Mesh, renderProxy, false, LODIndex);
		Collector.AddMesh(ViewIndex, Mesh);
	}

}