#include "ComputeDispatchers.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RHICommandList.h"
#include "RenderGraphBuilder.h"
#include "RenderTargetPool.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "ComputeDispatchersModule"

void FComputeDispatchersModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("VoxelRendering"))->GetBaseDir(), TEXT("Shaders/Private/ComputePasses"));
	AddShaderSourceDirectoryMapping(TEXT("/ComputeDispatchersShaders"), PluginShaderDir);
}

void FComputeDispatchersModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FComputeDispatchersModule, ComputeDispatchers)