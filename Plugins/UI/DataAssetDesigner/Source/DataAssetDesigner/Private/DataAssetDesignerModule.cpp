#include "DataAssetDesignerModule.h"
#include "UniversalDataAssetDetails.h"
#include "PropertyEditorModule.h"
#include "Engine/DataAsset.h"

#define LOCTEXT_NAMESPACE "FDataAssetDesignerModule"

void FDataAssetDesignerModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Register our custom layout for UDataAsset and all its children
	PropertyModule.RegisterCustomClassLayout(
		UDataAsset::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FUniversalDataAssetDetails::MakeInstance)
	);
}

void FDataAssetDesignerModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(UDataAsset::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDataAssetDesignerModule, DataAssetDesigner)
