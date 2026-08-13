#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"

class FUniversalDataAssetDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	FUniversalDataAssetDetails();

private:
	void AddCustomHeader(IDetailLayoutBuilder& DetailBuilder, FName CategoryName, FText Title, FLinearColor Color);
	void CreateDashboard(IDetailLayoutBuilder& DetailBuilder, const TArray<FString>& TopCategories, const FString& ObjectPath, UObject* TargetObject);

	FName ActiveTab;
	bool bShowValidationWarnings;

	FLinearColor GetColorForCategory(const FString& CategoryName);
	TMap<FString, FLinearColor> CategoryColors;
};
