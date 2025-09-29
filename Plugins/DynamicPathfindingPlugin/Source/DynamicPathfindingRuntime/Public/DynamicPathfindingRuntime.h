#pragma once

#include "Modules/ModuleManager.h"

class FDynamicPathfindingRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
