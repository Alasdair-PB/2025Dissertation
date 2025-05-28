#include "VoxelRenderingUtils.h"
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

void FVoxelRenderingUtilsModule::StartupModule(){}
void FVoxelRenderingUtilsModule::ShutdownModule(){}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FVoxelRenderingUtilsModule, VoxelRenderingUtils)

