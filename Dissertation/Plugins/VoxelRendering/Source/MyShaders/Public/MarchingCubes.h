#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "VoxelRenderingStructDef.h"
#include "MarchingCubes.generated.h"

struct MYSHADERS_API FMarchingCubesDispatchParams
{
	TArray<FOctreeNode> OctreeNodes;
	TArray<float> ScalarField;
	TArray<FTriangle> OutputTriangles;

	int X;
	int Y;
	int Z;
	float isoLevel;

	FMarchingCubesDispatchParams(int x, int y, int z):
		X(x),
		Y(y), 
		Z(z) {}
};

// This is a public interface that we define so outside code can invoke our compute shader.
class MYSHADERS_API FMarchingCubesInterface {
public:
	// Executes this shader on the render thread
	static void DispatchRenderThread(
		FRHICommandListImmediate& RHICmdList,
		FMarchingCubesDispatchParams Params,
		TFunction<void(TArray<FTriangle> OutputVal)> AsyncCallback
	);

	// Executes this shader on the render thread from the game thread via EnqueueRenderThreadCommand
	static void DispatchGameThread(
		FMarchingCubesDispatchParams Params,
		TFunction<void(TArray<FTriangle> OutputVal)> AsyncCallback
	)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
			{
				DispatchRenderThread(RHICmdList, Params, AsyncCallback);
			});
	}

	// Dispatches this shader. Can be called from any thread
	static void Dispatch(
		FMarchingCubesDispatchParams Params,
		TFunction<void(TArray<FTriangle> OutputVal)> AsyncCallback
	)
	{
		if (IsInRenderingThread())
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		else
			DispatchGameThread(Params, AsyncCallback);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMarchingCubesLibrary_AsyncExecutionCompleted, const TArray<FTriangle>, Value);

UCLASS()
class MYSHADERS_API UMarchingCubesLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	// Execute the actual load
	virtual void Activate() override {
		// Create a dispatch parameters struct and fill it the input array with our args
		FMarchingCubesDispatchParams Params(1, 1, 1);
		Params.OctreeNodes = OctreeNodes;
		Params.ScalarField = ScalarField;
		Params.isoLevel = isoLevel;

		// Dispatch the compute shader and wait until it completes
		FMarchingCubesInterface::Dispatch(Params, [this](TArray<FTriangle> OutputTriangles) {
			this->Completed.Broadcast(OutputTriangles);
			});
	}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UMarchingCubesLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, 
		TArray<FOctreeNode> OctreeNodes, 
		TArray<float> ScalarField,
		float isoLevel) 
	{

		UMarchingCubesLibrary_AsyncExecution* Action = NewObject<UMarchingCubesLibrary_AsyncExecution>();

		Action->OctreeNodes = OctreeNodes;
		Action->ScalarField = ScalarField;
		Action->isoLevel = isoLevel;
		Action->RegisterWithGameInstance(WorldContextObject);

		return Action;
	}

	UPROPERTY(BlueprintAssignable)
	FOnMarchingCubesLibrary_AsyncExecutionCompleted Completed;

	TArray<FOctreeNode> OctreeNodes;
	TArray<float> ScalarField;
	float isoLevel;
};