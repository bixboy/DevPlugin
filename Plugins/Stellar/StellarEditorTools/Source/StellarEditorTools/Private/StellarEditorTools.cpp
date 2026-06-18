#include "StellarEditorTools.h"
#include "PropertyEditorModule.h"
#include "Customizations/VehicleDataAssetDetails.h"
#include "Customizations/VehicleStructCustomizations.h"
#include "Data/VehicleData.h"

#define LOCTEXT_NAMESPACE "FStellarEditorToolsModule"

void FStellarEditorToolsModule::StartupModule()
{
	// Register custom detail layout for Vehicle Data Asset
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	
	PropertyModule.RegisterCustomClassLayout(
		UVehicleDataAsset::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FVehicleDataAssetDetails::MakeInstance)
	);

	// Register struct customizations for readable arrays
	PropertyModule.RegisterCustomPropertyTypeLayout(
		"VehicleSeatConfig",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FVehicleSeatConfigCustomization::MakeInstance)
	);

	PropertyModule.RegisterCustomPropertyTypeLayout(
		"VehicleTurretConfig",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FVehicleTurretConfigCustomization::MakeInstance)
	);
}

void FStellarEditorToolsModule::ShutdownModule()
{
	// Unregister on shutdown
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(UVehicleDataAsset::StaticClass()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout("VehicleSeatConfig");
		PropertyModule.UnregisterCustomPropertyTypeLayout("VehicleTurretConfig");
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FStellarEditorToolsModule, StellarEditorTools)
