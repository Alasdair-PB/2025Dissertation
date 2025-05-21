#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "RenderResource.h"
#include "VertexFactory.h"
#include "RHI.h"
#include "RHIResources.h"
#include "UniformBuffer.h"
#include "PrimitiveSceneProxy.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "SceneManagement.h"
#include "FVoxelVertexFactory.h"

/*
struct FVoxelProxySection
{
public:
	UMaterialInterface* Material;

	FStaticMeshVertexBuffers VertexBuffers;
	FRawStaticIndexBuffer IndexBuffer;

	FLocalVertexFactory VertexFactory;
	bool bSectionVisible;

	FVoxelProxySection(ERHIFeatureLevel::Type InFeatureLevel) : Material(NULL), VertexFactory(InFeatureLevel, "FVoxelProxySection"), bSectionVisible(true) {}

	~FVoxelProxySection() {
		this->VertexBuffers.PositionVertexBuffer.ReleaseResource();
		this->VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
		this->VertexBuffers.ColorVertexBuffer.ReleaseResource();
		this->IndexBuffer.ReleaseResource();
		this->VertexFactory.ReleaseResource();
	}
};*/