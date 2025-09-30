#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

class FUltraFlowFieldRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FUltraFlowFieldRuntimeModule, UltraFlowFieldRuntime)

