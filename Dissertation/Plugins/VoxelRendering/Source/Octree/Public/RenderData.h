#pragma once
#include "CoreMinimal.h"
#include "OctreeNode.h"

struct FVoxelProxyUpdateDataNode
{
private:
	OctreeNode* DataNode;
public:
	uint8 leafDepth;
	int64 visiblePoints;
	TSharedPtr<class FVoxelVertexFactory> VertexFactory;

	FVoxelProxyUpdateDataNode() : FVoxelProxyUpdateDataNode(0, 0, nullptr) {}
	FVoxelProxyUpdateDataNode(uint8 inLeafDepth, int64 inVisiblePoints, OctreeNode* DataNode)
		: DataNode(DataNode)
		, leafDepth(inLeafDepth)
		, visiblePoints(inVisiblePoints)
		, VertexFactory(nullptr) {
	}

	bool BuildDataCache(bool bUseStaticBuffers, bool bUseRayTracing) {
		if (DataNode)
		{
			VertexFactory = DataNode->GetVertexFactory();
			DataNode = nullptr;
			return  true;
		}
		return false;
	}
};
