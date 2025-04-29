#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"

class FVoxelRenderingModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
