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
			typeBuffer = dataNode->GetTypeBuffer();
			leafDepth = dataNode->GetDepth();
			boundsCenter = dataNode->GetBounds().Center();
			dataNode = nullptr;
			return  true;
		}
		return false;
	}
};

struct FVoxelTransVoxelNodeData {
private:
	TransitionCell* transitionCell;
	OctreeNode* owningNode;
public:
	FVoxelComputeUpdateNodeData highResolutionData[4];
	FVoxelComputeUpdateNodeData lowResolutionData;
	FIntVector direction;
	int transitionCellIndex;
	bool zeroNode;

	FVoxelTransVoxelNodeData() : FVoxelTransVoxelNodeData(nullptr, nullptr, 0) {}
	FVoxelTransVoxelNodeData(TransitionCell* inTransitionCell, OctreeNode* inOwner, int inTransitionCellIndex)
		: transitionCell(inTransitionCell), owningNode(inOwner), direction(FVector()), transitionCellIndex(inTransitionCellIndex), zeroNode(false)
{}
	FVoxelTransVoxelNodeData(OctreeNode* inOwner, int inNodeIndex)
		: transitionCell(nullptr), owningNode(inOwner), direction(FVector()), transitionCellIndex(inNodeIndex), zeroNode(true) {}

	bool BuildEmptyDataCache() {
		if (owningNode) {
			lowResolutionData = FVoxelComputeUpdateNodeData(owningNode);
			return lowResolutionData.BuildDataCache();
		}
		return false;
	}

	bool BuildDataCache() {
		if (zeroNode) return BuildEmptyDataCache();
		bool bReturnFlag = true;
		for (int i = 0; i < 4; i++) {
			if (transitionCell->adjacentNodes[i]) {
				OctreeNode* dataNodeTree = transitionCell->adjacentNodes[i];
				highResolutionData[i] = FVoxelComputeUpdateNodeData(dataNodeTree);
				bReturnFlag = bReturnFlag && highResolutionData[i].BuildDataCache();
			}
			else bReturnFlag = false;
		}
		if (owningNode) {
			lowResolutionData = FVoxelComputeUpdateNodeData(owningNode);
			bReturnFlag = bReturnFlag && lowResolutionData.BuildDataCache();
		} 
		else bReturnFlag = false;

		//UE_LOG(LogTemp, Warning, TEXT("Debug: assigning transvoxel direction: %d"), transitionCell->direction);
		direction = neighborOffsets[transitionCell->direction];
		//UE_LOG(LogTemp, Warning, TEXT("Debug: deformation pass occurs, direction = (%d, %d, %d)"), direction.X, direction.Y, direction.Z);
		return bReturnFlag;
	}
};

struct FVoxelComputeUpdateData {

private:
	Octree* octree;
public:
	float scale;
	float isoLevel;
	FVector3f octreePosition;
	int voxelsPerAxis;
	int highResVoxelsPerAxis;

	TSharedPtr<FIsoDynamicBuffer> deltaIsoBuffer;
	TSharedPtr<FTypeDynamicBuffer> deltaTypeBuffer;

	TSharedPtr<FIsoDynamicBuffer> zeroIsoBuffer;
	TSharedPtr<FTypeDynamicBuffer> zeroTypeBuffer;

	TSharedPtr<FIsoUniformBuffer> isoBuffer;
	TSharedPtr<FTypeUniformBuffer> typeBuffer;
	TArray<FVoxelComputeUpdateNodeData> nodeData;
	TArray<FVoxelTransVoxelNodeData> transVoxelNodeData;
	TSharedPtr<FMarchingCubesLookUpResource> marchLookUpResource;

	FVoxelComputeUpdateData() :FVoxelComputeUpdateData(nullptr) {}
	FVoxelComputeUpdateData(Octree* inOctree) : octree(inOctree), scale(0), isoLevel(0), octreePosition(FVector3f()), 
		voxelsPerAxis(0), highResVoxelsPerAxis(0) {}

	bool BuildDataCache() {

		if (!octree) return false;

		check(octree->GetIsoBuffer());
		check(octree->GetTypeBuffer());
		check(octree->GetDeltaIsoBuffer());
		check(octree->GetDeltaTypeBuffer());
		check(octree->GetMarchLookUpResourceBuffer());
		check(octree->GetZeroIsoBuffer());
		check(octree->GetZeroTypeBuffer());

		zeroIsoBuffer = octree->GetZeroIsoBuffer();
		zeroTypeBuffer = octree->GetZeroTypeBuffer();
		isoBuffer = octree->GetIsoBuffer();
		typeBuffer = octree->GetTypeBuffer();
		deltaIsoBuffer = octree->GetDeltaIsoBuffer();
		deltaTypeBuffer = octree->GetDeltaTypeBuffer();
		octreePosition = octree->GetOctreePosition();
		voxelsPerAxis = octree->GetVoxelsPerAxs();
		marchLookUpResource = octree->GetMarchLookUpResourceBuffer();
		scale = octree->GetScale();
		isoLevel = octree->GetIsoLevel();
		highResVoxelsPerAxis = octree->GetVoxelsPerAxsMaxRes();
		octree = nullptr;
		return  true;
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
		UE_LOG(LogTemp, Warning, TEXT("WARNING RENDER DATA HAS NOT BEEN BUILT"));

		return false;
	}
};
