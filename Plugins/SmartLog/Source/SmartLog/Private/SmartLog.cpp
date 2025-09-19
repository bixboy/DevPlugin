#include "SmartLog.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogSmart);

#define LOCTEXT_NAMESPACE "FSmartLogModule"

void FSmartLogModule::StartupModule()
{
	UE_LOG(LogSmart, Log, TEXT("SmartLog module started"));
}

void FSmartLogModule::ShutdownModule()
{
	UE_LOG(LogSmart, Log, TEXT("SmartLog module shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSmartLogModule, SmartLog);