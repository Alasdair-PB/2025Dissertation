#pragma once
#include "CoreMinimal.h"
#include "CommonRenderResources.h"
#include "RenderGraph.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "TextureGeneratorDispatcher.generated.h"


USTRUCT(BlueprintType)
struct FTextureGeneratorOutput
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TArray<float> outIsoValues;
	TArray<uint32> outTypeValues;

};

USTRUCT(BlueprintType)
struct FTextureGeneratorInput
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int size = 0;
	UPROPERTY(BlueprintReadOnly) int seed = 0;
	UPROPERTY(BlueprintReadOnly) float baseDepthScale = 0;
	UPROPERTY(BlueprintReadOnly) float isoLevel = 0;
	UPROPERTY(BlueprintReadOnly) float planetScaleRatio = 0;
};

struct COMPUTEDISPATCHERS_API FTextureGeneratorDispatchParams
{
	int X = 1;
	int Y = 1;
	int Z = 1;

	FTextureGeneratorInput Input;
	FTextureGeneratorOutput Output;

	FTextureGeneratorDispatchParams(int x, int y, int z) :
		X(x),Y(y),Z(z){}
};

class COMPUTEDISPATCHERS_API FTextureGeneratorInterface {
public:
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList,
		FTextureGeneratorDispatchParams Params,TFunction<void(FTextureGeneratorOutput OutputVal)> AsyncCallback);

	static void DispatchGameThread(FTextureGeneratorDispatchParams Params,TFunction<void(FTextureGeneratorOutput OutputVal)> AsyncCallback)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
				{ DispatchRenderThread(RHICmdList, Params, AsyncCallback);});
	}

	static void Dispatch(FTextureGeneratorDispatchParams Params,TFunction<void(FTextureGeneratorOutput OutputVal)> AsyncCallback)
	{
		if (IsInRenderingThread())
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		else
			DispatchGameThread(Params, AsyncCallback);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTextureGeneratorLibrary_AsyncExecutionCompleted, const FTextureGeneratorOutput, Value);

UCLASS()
class COMPUTEDISPATCHERS_API UTextureGeneratorLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override {
		FTextureGeneratorDispatchParams Params(1, 1, 1);
		Params.Input.size = Args.size;
		Params.Input.baseDepthScale = Args.baseDepthScale;

		FTextureGeneratorInterface::Dispatch(Params, [this](FTextureGeneratorOutput OutputVal) {
			this->Completed.Broadcast(OutputVal);
			});
	}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UTextureGeneratorLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, FTextureGeneratorInput Args) {
		UTextureGeneratorLibrary_AsyncExecution* Action = NewObject<UTextureGeneratorLibrary_AsyncExecution>();
		Action->Args = Args;
		Action->RegisterWithGameInstance(WorldContextObject);
		return Action;
	}

	UPROPERTY(BlueprintAssignable)
	FOnTextureGeneratorLibrary_AsyncExecutionCompleted Completed;
	FTextureGeneratorInput Args;
};