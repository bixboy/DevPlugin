#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * Custom detail panel for UVehicleDataAsset to provide a premium, structured editor experience.
 */
class FVehicleDataAssetDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this customization */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	/** Helper to create a stylized section header */
	void AddCustomHeader(IDetailLayoutBuilder& DetailBuilder, FName CategoryName, FText Title, FLinearColor Color, FName IconName);
};
