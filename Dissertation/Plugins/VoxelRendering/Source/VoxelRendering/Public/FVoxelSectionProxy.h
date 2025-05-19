#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "RenderResource.h"
#include "VertexFactory.h"
#include "LocalVertexFactory.h"
#include "RHI.h"
#include "RHIResources.h"
#include "UniformBuffer.h"
#include "PrimitiveSceneProxy.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "SceneManagement.h"
#include "FVoxelVertexFactory.h"

class FVoxelSectionProxy {
public:
	UMaterialInterface* Material;
	FRawStaticIndexBuffer IndexBuffer;
	FVoxelVertexFactory VertexFactory;

	bool bSectionVisible;
	uint32 MaxVertexIndex;

	FVoxelSectionProxy(ERHIFeatureLevel::Type InFeatureLevel)
		: Material(NULL)
		, VertexFactory(InFeatureLevel)
		, bSectionVisible(true)
	{}
};
