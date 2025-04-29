#include "MyShaders.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RHICommandList.h"
#include "RenderGraphBuilder.h"
#include "RenderTargetPool.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "MyShadersModule"

void FMyShadersModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("VoxelRendering"))->GetBaseDir(), TEXT("Shaders/Private"));
	AddShaderSourceDirectoryMapping(TEXT("/MyShadersShaders"), PluginShaderDir);
}

void FMyShadersModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FMyShadersModule, MyShaders)