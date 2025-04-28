#pragma once
#include "Modules/ModuleManager.h"

class FOctreeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};