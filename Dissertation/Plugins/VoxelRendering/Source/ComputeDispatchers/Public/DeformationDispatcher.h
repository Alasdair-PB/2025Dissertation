#pragma once
#include "CoreMinimal.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "DeformationDispatcher.generated.h"


USTRUCT(BlueprintType)
struct FDeformationOutput
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TArray<float> outIsoValues;
	TArray<uint32> outTypeValues;

};

USTRUCT(BlueprintType)
struct FDeformationInput
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int size = 0;
	UPROPERTY(BlueprintReadOnly) int seed = 0;
	UPROPERTY(BlueprintReadOnly) float baseDepthScale = 0;
	UPROPERTY(BlueprintReadOnly) float isoLevel = 0;
	UPROPERTY(BlueprintReadOnly) float planetScaleRatio = 0;
};

struct COMPUTEDISPATCHERS_API FDeformationDispatchParams
{
	int X = 1;
	int Y = 1;
	int Z = 1;

	FDeformationInput Input;
	FDeformationOutput Output;

	FDeformationDispatchParams(int x, int y, int z) :
		X(x),Y(y),Z(z){}
};

class COMPUTEDISPATCHERS_API FPlanetGeneratorInterface {
public:
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList,
		FDeformationDispatchParams Params,TFunction<void(FDeformationOutput OutputVal)> AsyncCallback);

	static void DispatchGameThread(FDeformationDispatchParams Params,TFunction<void(FDeformationOutput OutputVal)> AsyncCallback)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
				{ DispatchRenderThread(RHICmdList, Params, AsyncCallback);});
	}

	static void Dispatch(FDeformationDispatchParams Params,TFunction<void(FDeformationOutput OutputVal)> AsyncCallback)
	{
		if (IsInRenderingThread())
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		else
			DispatchGameThread(Params, AsyncCallback);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeformationLibrary_AsyncExecutionCompleted, const FDeformationOutput, Value);

UCLASS()
class COMPUTEDISPATCHERS_API UDeformationLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override {
		FDeformationDispatchParams Params(1, 1, 1);
		Params.Input.size = Args.size;
		Params.Input.baseDepthScale = Args.baseDepthScale;

		FPlanetGeneratorInterface::Dispatch(Params, [this](FDeformationOutput OutputVal) {
			this->Completed.Broadcast(OutputVal);
			});
	}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UDeformationLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, FDeformationInput Args) {
		UDeformationLibrary_AsyncExecution* Action = NewObject<UDeformationLibrary_AsyncExecution>();
		Action->Args = Args;
		Action->RegisterWithGameInstance(WorldContextObject);
		return Action;
	}

	UPROPERTY(BlueprintAssignable)
	FOnDeformationLibrary_AsyncExecutionCompleted Completed;
	FDeformationInput Args;
};