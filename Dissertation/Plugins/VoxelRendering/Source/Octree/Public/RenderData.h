#pragma once
#include "CoreMinimal.h"
#include "OctreeNode.h"
#include "Octree.h"

/**
 * Update data for Compute dispatch
 */

struct FVoxelComputeUpdateNodeData {
private:
	OctreeNode* dataNode;
public:
	int leafDepth;
	FVector3f boundsCenter;

	TSharedPtr<class FVoxelVertexFactory> vertexFactory;
	TSharedPtr<FIsoDynamicBuffer> isoBuffer;
	TSharedPtr<FTypeDynamicBuffer> typeBuffer;

	FVoxelComputeUpdateNodeData() : FVoxelComputeUpdateNodeData(nullptr) {}
	FVoxelComputeUpdateNodeData(OctreeNode* inDataNode)
		: dataNode(inDataNode)
		, leafDepth(0)
		, boundsCenter(FVector3f())
		, vertexFactory(nullptr) {
	}

	bool BuildDataCache() {
		if (dataNode)
		{
			vertexFactory = dataNode->GetVertexFactory();
			isoBuffer = dataNode->GetIsoBuffer();
			leafDepth = dataNode->GetDepth();
			boundsCenter = dataNode->GetBounds().Center();
			dataNode = nullptr;
			return  true;
		}
		return false;
	}
};


struct FVoxelComputeUpdateData {

private:
	Octree* octree;
public:

	FVoxelComputeUpdateData() :FVoxelComputeUpdateData(nullptr) {}
	FVoxelComputeUpdateData(Octree* inOctree) : octree(inOctree), scale(0), isoLevel(0), 
		highResVoxelsPerAxis(0), voxelsPerAxis(0), octreePosition(FVector3f()) {}

	float scale;
	float isoLevel;
	FVector3f octreePosition;
	int voxelsPerAxis;
	int highResVoxelsPerAxis;

	bool BuildDataCache() {

		if (!octree) return false;

		check(octree->GetIsoBuffer());
		check(octree->GetTypeBuffer());
		check(octree->GetDeltaIsoBuffer());

		isoBuffer = octree->GetIsoBuffer();
		typeBuffer = octree->GetTypeBuffer();
		deltaIsoBuffer = octree->GetDeltaIsoBuffer();

		octreePosition = octree->GetOctreePosition();
		voxelsPerAxis = octree->GetVoxelsPerAxs();
		scale = octree->GetScale();
		isoLevel = octree->GetIsoLevel();
		highResVoxelsPerAxis = octree->GetVoxelsPerAxsMaxRes();
		octree = nullptr;
		return  true;
	}

	TSharedPtr<FIsoDynamicBuffer> deltaIsoBuffer;
	TSharedPtr<FIsoUniformBuffer> isoBuffer;
	TSharedPtr<FTypeUniformBuffer> typeBuffer;
	TArray<FVoxelComputeUpdateNodeData> nodeData;
};

/**
  * Update data for SceneProxy
 */

struct FVoxelProxyUpdateDataNode
{
private:
	OctreeNode* dataNode;
public:
	uint8 leafDepth;
	TSharedPtr<class FVoxelVertexFactory> vertexFactory;

	FVoxelProxyUpdateDataNode() : FVoxelProxyUpdateDataNode(0, nullptr) {}
	FVoxelProxyUpdateDataNode(uint8 inLeafDepth, OctreeNode* inDataNode)
		: dataNode(inDataNode)
		, leafDepth(inLeafDepth)
		, vertexFactory(nullptr) {
	}

	bool BuildDataCache() {
		if (dataNode)
		{
			vertexFactory = dataNode->GetVertexFactory();
			dataNode = nullptr;
			return  true;
		}
		UE_LOG(LogTemp, Warning, TEXT("WARNING RENDER DATA HAS NOT BEEN BUILT"));

		return false;
	}
};
