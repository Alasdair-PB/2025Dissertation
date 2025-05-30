#include "VoxelRendering.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RHICommandList.h"
#include "RenderGraphBuilder.h"
#include "RenderTargetPool.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#define LOCTEXT_NAMESPACE "FVoxelRenderingModule"

void FVoxelRenderingModule::StartupModule()
{
	//FString VoxelRenderingShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("VoxelRendering"))->GetBaseDir(), TEXT("Shaders/Private/VertexFactories"));
	//AddShaderSourceDirectoryMapping(TEXT("/VertexFactoryShaders"), VoxelRenderingShaderDir);
}
void FVoxelRenderingModule::ShutdownModule(){}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FVoxelRenderingModule, VoxelRendering)

