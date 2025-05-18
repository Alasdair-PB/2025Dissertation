#include "MySimpleComputeShader.h"
#include "ComputeDispatchers.h"
#include "ComputeDispatchers/Public/MySimpleComputeShader.h"
#include "PixelShaderUtils.h"
#include "Runtime/RenderCore/Public/RenderGraphUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MaterialShader.h"

DECLARE_STATS_GROUP(TEXT("MySimpleComputeShader"), STATGROUP_MySimpleComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("MySimpleComputeShader Execute"), STAT_MySimpleComputeShader_Execute, STATGROUP_MySimpleComputeShader);

class COMPUTEDISPATCHERS_API FMySimpleComputeShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FMySimpleComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FMySimpleComputeShader, FGlobalShader);

	class FMySimpleComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<FMySimpleComputeShader_Perm_TEST>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, Input)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<int>, Output)
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_MySimpleComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_MySimpleComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_MySimpleComputeShader_Z);
	}
private:
};

IMPLEMENT_GLOBAL_SHADER(FMySimpleComputeShader, "/ComputeDispatchersShaders/MySimpleComputeShader.usf", "MySimpleComputeShader", SF_Compute);

void FMySimpleComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FMySimpleComputeShaderDispatchParams Params, TFunction<void(int OutputVal)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_MySimpleComputeShader_Execute);
		DECLARE_GPU_STAT(MySimpleComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "MySimpleComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, MySimpleComputeShader);

		typename FMySimpleComputeShader::FPermutationDomain PermutationVector;
		TShaderMapRef<FMySimpleComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			FMySimpleComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FMySimpleComputeShader::FParameters>();
			const void* RawData = (void*)Params.Input;
			int NumInputs = 2;
			int InputSize = sizeof(int);
			FRDGBufferRef InputBuffer = CreateUploadBuffer(GraphBuilder, TEXT("InputBuffer"), InputSize, NumInputs, RawData, InputSize * NumInputs);
			PassParameters->Input = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(InputBuffer, PF_R32_SINT));

			FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateBufferDesc(sizeof(int32), 1),
				TEXT("OutputBuffer"));

			PassParameters->Output = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_SINT));

			auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), FComputeShaderUtils::kGolden2DGroupSize);
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteMySimpleComputeShader"), PassParameters, ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList){
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
				});

			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteMySimpleComputeShaderOutput"));
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, OutputBuffer, 0u);

			auto RunnerFunc = [GPUBufferReadback, AsyncCallback](auto&& RunnerFunc) -> void {
				if (GPUBufferReadback->IsReady()) {
					int32* Buffer = (int32*)GPUBufferReadback->Lock(0);
					int OutVal = Buffer[0];
					GPUBufferReadback->Unlock();
					AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutVal]() {AsyncCallback(OutVal);});
					delete GPUBufferReadback;
				}
				else {
					AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
						RunnerFunc(RunnerFunc);
						});
				}};

			AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
				RunnerFunc(RunnerFunc);});
		}
		else {}
	}

	GraphBuilder.Execute();
}