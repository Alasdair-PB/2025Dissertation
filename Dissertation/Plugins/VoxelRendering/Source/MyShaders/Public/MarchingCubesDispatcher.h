#pragma once
#include "CoreMinimal.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "../../Octree/Public/OctreeNode.h"
#include "MarchingCubesDispatcher.generated.h"


USTRUCT(BlueprintType)
struct FMarchingCubesOutput
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TArray<FVector3f> vertices;
	UPROPERTY(BlueprintReadOnly) TArray<int32> tris;
	UPROPERTY(BlueprintReadOnly) TArray<FVector3f> normals;
};

USTRUCT(BlueprintType)
struct FMarchingCubesInput
{
	GENERATED_BODY()
	OctreeNode* tree;
	UPROPERTY(BlueprintReadOnly) int leafCount;
	UPROPERTY(BlueprintReadOnly) FVector leafPosition;
	UPROPERTY(BlueprintReadOnly) int leafDepth;
	UPROPERTY(BlueprintReadOnly) int nodeIndex;
	UPROPERTY(BlueprintReadOnly) int voxelsPerAxis;
	UPROPERTY(BlueprintReadOnly) float baseDepthScale;
	UPROPERTY(BlueprintReadOnly) float isoLevel;
	UPROPERTY(BlueprintReadOnly) TArray<float> isoValues;
};

struct MYSHADERS_API FMarchingCubesDispatchParams
{
	int X;
	int Y;
	int Z;

	FMarchingCubesInput Input;
	FMarchingCubesOutput Output;

	FMarchingCubesDispatchParams(int x, int y, int z) :
		X(x),Y(y),Z(z){}
};

class MYSHADERS_API FMarchingCubesInterface {
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
class MYSHADERS_API UMarchingCubesLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override {
		FMarchingCubesDispatchParams Params(1, 1, 1);
		Params.Input.leafDepth = Args.leafDepth;
		Params.Input.isoValues = Args.isoValues;
		Params.Input.leafPosition = Args.leafPosition;
		Params.Input.tree = Args.tree;
		Params.Input.leafCount = Args.leafCount;

		Params.Input.voxelsPerAxis = Args.voxelsPerAxis;
		Params.Input.nodeIndex = Args.nodeIndex;
		Params.Input.baseDepthScale = Args.baseDepthScale;
		Params.Input.isoLevel = Args.isoLevel;

		FMarchingCubesInterface::Dispatch(Params, [this](FMarchingCubesOutput OutputVal) {
			this->Completed.Broadcast(OutputVal);
			});
	}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UMarchingCubesLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, FMarchingCubesInput Args) {
		UMarchingCubesLibrary_AsyncExecution* Action = NewObject<UMarchingCubesLibrary_AsyncExecution>();
		Action->Args = Args;
		Action->RegisterWithGameInstance(WorldContextObject);
		return Action;
	}

	UPROPERTY(BlueprintAssignable)
	FOnMarchingCubesLibrary_AsyncExecutionCompleted Completed;
	FMarchingCubesInput Args;
};