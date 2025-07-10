#pragma once
#include "CoreMinimal.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "../../Octree/Public/OctreeNode.h"
#include "RenderGraphResources.h"
#include "RHI.h"
#include "RenderData.h"
#include "MarchingCubesDispatcher.generated.h"

USTRUCT(BlueprintType)
struct FMarchingCubesOutput
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FMarchingCubesInput
{
	GENERATED_BODY()
public:
	FVoxelComputeUpdateData Input;
};

struct COMPUTEDISPATCHERS_API FMarchingCubesDispatchParams
{
	int X = 1;
	int Y = 1;
	int Z = 1;

	FVoxelComputeUpdateData Input;
	FMarchingCubesOutput Output;

	FMarchingCubesDispatchParams(int x, int y, int z) :
		X(x),Y(y),Z(z){}
};

class COMPUTEDISPATCHERS_API FMarchingCubesInterface {
public:
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList,
		FMarchingCubesDispatchParams Params,TFunction<void(FMarchingCubesOutput OutputVal)> AsyncCallback);

	static void DispatchGameThread(FMarchingCubesDispatchParams Params,TFunction<void(FMarchingCubesOutput OutputVal)> AsyncCallback)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
				{ DispatchRenderThread(RHICmdList, Params, AsyncCallback);});
	}

	static void Dispatch(FMarchingCubesDispatchParams Params,TFunction<void(FMarchingCubesOutput OutputVal)> AsyncCallback)
	{
		if (IsInRenderingThread())
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		else
			DispatchGameThread(Params, AsyncCallback);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMarchingCubesLibrary_AsyncExecutionCompleted, const FMarchingCubesOutput, Value);

UCLASS()
class COMPUTEDISPATCHERS_API UMarchingCubesLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override {
		FMarchingCubesDispatchParams Params(1, 1, 1);
		Params.Input.isoBuffer = Args.isoBuffer;
		Params.Input.isoLevel = Args.isoLevel;
		Params.Input.nodeData = Args.nodeData;

		Params.Input.scale = Args.scale;
		Params.Input.typeBuffer = Args.typeBuffer;
		Params.Input.voxelsPerAxis = Args.voxelsPerAxis;

		FMarchingCubesInterface::Dispatch(Params, [this](FMarchingCubesOutput OutputVal) {
			this->Completed.Broadcast(OutputVal);
			});
	}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UMarchingCubesLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, FVoxelComputeUpdateData Args) {
		UMarchingCubesLibrary_AsyncExecution* Action = NewObject<UMarchingCubesLibrary_AsyncExecution>();
		Action->Args = Args;
		Action->RegisterWithGameInstance(WorldContextObject);
		return Action;
	}

	UPROPERTY(BlueprintAssignable)
	FOnMarchingCubesLibrary_AsyncExecutionCompleted Completed;
	FVoxelComputeUpdateData Args;
};