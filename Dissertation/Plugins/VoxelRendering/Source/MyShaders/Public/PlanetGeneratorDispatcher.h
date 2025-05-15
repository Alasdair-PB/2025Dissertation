#pragma once
#include "CoreMinimal.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "PlanetGeneratorDispatcher.generated.h"


USTRUCT(BlueprintType)
struct FPlanetGeneratorOutput
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TArray<float> outIsoValues;
};

USTRUCT(BlueprintType)
struct FPlanetGeneratorInput
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int size = 0;
	UPROPERTY(BlueprintReadOnly) float baseDepthScale = 0;
	UPROPERTY(BlueprintReadOnly) FVector3f objectPosition = FVector3f();

};

struct MYSHADERS_API FPlanetGeneratorDispatchParams
{
	int X = 1;
	int Y = 1;
	int Z = 1;

	FPlanetGeneratorInput Input;
	FPlanetGeneratorOutput Output;

	FPlanetGeneratorDispatchParams(int x, int y, int z) :
		X(x),Y(y),Z(z){}
};

class MYSHADERS_API FPlanetGeneratorInterface {
public:
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList,
		FPlanetGeneratorDispatchParams Params,TFunction<void(FPlanetGeneratorOutput OutputVal)> AsyncCallback);

	static void DispatchGameThread(FPlanetGeneratorDispatchParams Params,TFunction<void(FPlanetGeneratorOutput OutputVal)> AsyncCallback)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
				{ DispatchRenderThread(RHICmdList, Params, AsyncCallback);});
	}

	static void Dispatch(FPlanetGeneratorDispatchParams Params,TFunction<void(FPlanetGeneratorOutput OutputVal)> AsyncCallback)
	{
		if (IsInRenderingThread())
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		else
			DispatchGameThread(Params, AsyncCallback);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlanetGeneratorLibrary_AsyncExecutionCompleted, const FPlanetGeneratorOutput, Value);

UCLASS()
class MYSHADERS_API UPlanetGeneratorLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override {
		FPlanetGeneratorDispatchParams Params(1, 1, 1);
		Params.Input.size = Args.size;
		Params.Input.baseDepthScale = Args.baseDepthScale;

		FPlanetGeneratorInterface::Dispatch(Params, [this](FPlanetGeneratorOutput OutputVal) {
			this->Completed.Broadcast(OutputVal);
			});
	}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UPlanetGeneratorLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, FPlanetGeneratorInput Args) {
		UPlanetGeneratorLibrary_AsyncExecution* Action = NewObject<UPlanetGeneratorLibrary_AsyncExecution>();
		Action->Args = Args;
		Action->RegisterWithGameInstance(WorldContextObject);
		return Action;
	}

	UPROPERTY(BlueprintAssignable)
	FOnPlanetGeneratorLibrary_AsyncExecutionCompleted Completed;
	FPlanetGeneratorInput Args;
};