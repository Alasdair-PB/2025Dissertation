#pragma once
#include "CoreMinimal.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "MarchingCubesDispatcher.generated.h"

USTRUCT(BlueprintType)
struct FMarchingCubesOutput
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly)
	TArray<FVector3f> vertices;
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> tris;
	UPROPERTY(BlueprintReadOnly)
	TArray<FVector3f> normals;
};

USTRUCT(BlueprintType)
struct FMarchingCubesInput
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly)
	TArray<FVector3f> voxelBodyCoord;
	UPROPERTY(BlueprintReadOnly)
	TArray<float> scalarFieldOffset;
	UPROPERTY(BlueprintReadOnly)
	TArray<float> depthLevel;
	UPROPERTY(BlueprintReadOnly)
	float isoLevel;
};

struct MYSHADERS_API FMarchingCubesDispatchParams
{
	int X;
	int Y;
	int Z;

	FVector voxelBodyDimensions;
	FMarchingCubesInput Input;
	FMarchingCubesOutput Output;

	FMarchingCubesDispatchParams(int x, int y, int z, FVector voxelBodyDimensions) :
		X(x),Y(y),Z(z), voxelBodyDimensions(voxelBodyDimensions){}
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
		FMarchingCubesDispatchParams Params(1, 1, 1, FVector(1,1,1));
		Params.Input.depthLevel = Args.depthLevel;
		Params.Input.voxelBodyCoord = Args.voxelBodyCoord;
		Params.Input.scalarFieldOffset = Args.scalarFieldOffset;
		Params.Input.depthLevel = Args.depthLevel;

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