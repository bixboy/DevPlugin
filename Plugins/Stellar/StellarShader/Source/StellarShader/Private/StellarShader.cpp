#include "StellarShader.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FStellarShaderModule"

void FStellarShaderModule::StartupModule()
{
	const FString ShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("StellarShader"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/StellarShader"), ShaderDir);
}

void FStellarShaderModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FStellarShaderModule, StellarShader)