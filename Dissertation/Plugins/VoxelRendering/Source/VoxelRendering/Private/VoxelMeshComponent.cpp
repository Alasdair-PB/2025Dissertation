#include "VoxelMeshComponent.h"
#include "FVoxelVertexFactory.h"
#include "FVoxelSceneProxy.h"

#include "FVoxelVertexFactoryShaderParameters.h"

IMPLEMENT_TYPE_LAYOUT(FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelVertexFactory, SF_Vertex, FVoxelVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_TYPE(FVoxelVertexFactory, "/VertexFactoryShaders/VoxelVertexFactory.ush", false, false, false, false, false);

FPrimitiveSceneProxy* UVoxelMeshComponent::CreateSceneProxy()
{
	if (!SceneProxy)
		return new FVoxelSceneProxy();
	else
		return SceneProxy;
}