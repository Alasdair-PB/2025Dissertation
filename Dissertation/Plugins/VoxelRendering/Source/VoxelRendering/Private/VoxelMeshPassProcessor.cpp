#include "VoxelMeshPassProcessor.h"

FMeshPassProcessor* CreateVoxelPassProcessor(ERHIFeatureLevel::Type FeatureLevel, const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
{
	return new FVoxelMeshPassProcessor(Scene, InViewIfDynamicMeshCommand, InDrawListContext);
}

// VoxelInfoPass added to source file: MeshPassProcessor.h
// 	case EMeshPass::VoxelInfoPass: return TEXT("VoxelInfoPass");
// change 38 to 39
FRegisterPassProcessorCreateFunction RegisterVoxelInfoPass(&CreateVoxelPassProcessor, EShadingPath::Deferred, EMeshPass::VoxelInfoPass, EMeshPassFlags::MainView);