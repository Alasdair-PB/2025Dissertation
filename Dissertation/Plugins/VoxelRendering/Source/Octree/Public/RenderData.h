#pragma once
#include "CoreMinimal.h"
#include "OctreeNode.h"
#include "Octree.h"

struct FVoxelComputeUpdateData {

private:
	Octree* octree;
public:

	FVoxelComputeUpdateData() :FVoxelComputeUpdateData(nullptr) {}
	FVoxelComputeUpdateData(Octree* inOctree) : octree(inOctree), scale(0), isoLevel(0), voxelsPerAxis(0) {}

	float scale;
	float isoLevel;
	int voxelsPerAxis;

	bool BuildDataCache() {
		if (!octree) return false;
		
		isoBuffer = octree->GetIsoBuffer();
		typeBuffer = octree->GetTypeBuffer();

		for (FVoxelComputeUpdateNodeData node : nodeData) {
			if (!node.BuildDataCache()) return false;
		}

		voxelsPerAxis = octree->GetVoxelsPerAxs();
		scale = octree->GetScale();
		isoLevel = octree->GetIsoLevel();

		octree = nullptr;
		return  true;
	}

	TSharedPtr<FIsoUniformBuffer> isoBuffer;
	TSharedPtr<FTypeUniformBuffer> typeBuffer;
	TArray<FVoxelComputeUpdateNodeData> nodeData;
};

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

	FVoxelComputeUpdateNodeData() : FVoxelComputeUpdateNodeData(0, nullptr) {}
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
		return false;
	}
};
