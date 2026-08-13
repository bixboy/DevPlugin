#include "UniversalDataAssetDetails.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Styling/SlateIconFinder.h"
#include "IPropertyUtilities.h"
#include "Misc/ConfigCacheIni.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Layout/SSeparator.h"

#define LOCTEXT_NAMESPACE "UniversalDataAssetDetails"

FUniversalDataAssetDetails::FUniversalDataAssetDetails()
{
	ActiveTab = NAME_None;
	bShowValidationWarnings = false;

	CategoryColors.Add(TEXT("Walker"), FLinearColor(0.3f, 0.7f, 1.0f));
	CategoryColors.Add(TEXT("Vehicle"), FLinearColor(0.3f, 0.7f, 1.0f));
	CategoryColors.Add(TEXT("Ship"), FLinearColor(0.3f, 0.7f, 1.0f));
	CategoryColors.Add(TEXT("Core"), FLinearColor(1.0f, 0.4f, 0.4f));
	CategoryColors.Add(TEXT("Setup"), FLinearColor(1.0f, 0.7f, 0.3f));
	CategoryColors.Add(TEXT("Mechanics"), FLinearColor(0.4f, 0.9f, 0.5f));
	CategoryColors.Add(TEXT("Optimization"), FLinearColor(0.8f, 0.5f, 1.0f));
	CategoryColors.Add(TEXT("Network"), FLinearColor(0.3f, 0.9f, 0.9f));
}

TSharedRef<IDetailCustomization> FUniversalDataAssetDetails::MakeInstance()
{
	return MakeShareable(new FUniversalDataAssetDetails);
}

FLinearColor FUniversalDataAssetDetails::GetColorForCategory(const FString& CategoryName)
{
	if (FLinearColor* FoundColor = CategoryColors.Find(CategoryName))
	{
		return *FoundColor;
	}
	
	// Fallback deterministic pseudo-random color based on string length and hash
	uint32 Hash = GetTypeHash(CategoryName);
	float Hue = (Hash % 360) / 360.0f;
	return FLinearColor::MakeFromHSV8(Hue * 255.f, 160, 255);
}

void FUniversalDataAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

	if (CustomizedObjects.Num() == 0 || !CustomizedObjects[0].IsValid())
		return;

	UObject* TargetObject = CustomizedObjects[0].Get();
	FString ObjectPath = TargetObject->GetPathName();
	bool bIsPremiumEnabled = true;
	GConfig->GetBool(TEXT("DataAssetDesigner"), *ObjectPath, bIsPremiumEnabled, GEditorPerProjectIni);

	TSharedPtr<IPropertyUtilities> PropUtils = DetailBuilder.GetPropertyUtilities();

	UClass* ObjectClass = TargetObject->GetClass();

	// 1. Collect all top-level categories in native order
	TArray<FString> TopCategories;
	TSet<FString> SeenCategories;

	TArray<UClass*> ClassHierarchy;
	for (UClass* Class = ObjectClass; Class; Class = Class->GetSuperClass())
	{
		ClassHierarchy.Insert(Class, 0); // Base first
	}

	for (UClass* Class : ClassHierarchy)
	{
		for (TFieldIterator<FProperty> PropIt(Class, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
		{
			FProperty* Prop = *PropIt;
			if (!Prop->HasAnyPropertyFlags(CPF_Edit)) continue;

			FString FullCat = Prop->GetMetaData(TEXT("Category"));
			if (FullCat.IsEmpty()) FullCat = TEXT("General");

			FString TopLevel, SubCat;
			if (!FullCat.Split(TEXT("|"), &TopLevel, &SubCat))
				TopLevel = FullCat;

			if (!SeenCategories.Contains(TopLevel))
			{
				SeenCategories.Add(TopLevel);
				TopCategories.Add(TopLevel);
			}
		}
	}

	// Create Dashboard Header (Toggle, Validate Button, Tabs)
	CreateDashboard(DetailBuilder, TopCategories, ObjectPath, TargetObject);

	if (!bIsPremiumEnabled)
	{
		return; // Stop here and let Unreal natively render everything!
	}

	FString SavedActiveTab;
	GConfig->GetString(TEXT("DataAssetDesigner"), *(ObjectPath + TEXT("_ActiveTab")), SavedActiveTab, GEditorPerProjectIni);
	ActiveTab = FName(*SavedActiveTab);

	// Ensure ActiveTab is valid
	if (ActiveTab.IsNone() && TopCategories.Num() > 0)
	{
		ActiveTab = FName(*TopCategories[0]);
	}
	else if (!TopCategories.Contains(ActiveTab.ToString()))
	{
		ActiveTab = TopCategories.Num() > 0 ? FName(*TopCategories[0]) : NAME_None;
	}

	// 2. Hide all non-active categories natively, and style the active one
	for (const FString& Cat : TopCategories)
	{
		if (FName(*Cat) != ActiveTab)
		{
			DetailBuilder.HideCategory(FName(*Cat));
		}
	}
	AddCustomHeader(DetailBuilder, ActiveTab, FText::FromString(ActiveTab.ToString()), GetColorForCategory(ActiveTab.ToString()));

	for (UClass* Class : ClassHierarchy)
	{
		for (TFieldIterator<FProperty> PropIt(Class, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
		{
			FProperty* Prop = *PropIt;
			if (!Prop->HasAnyPropertyFlags(CPF_Edit)) continue;

			FString FullCat = Prop->GetMetaData(TEXT("Category"));
			if (FullCat.IsEmpty()) FullCat = TEXT("General");

			FString TopLevel, SubCat;
			if (!FullCat.Split(TEXT("|"), &TopLevel, &SubCat)) TopLevel = FullCat;

			TSharedRef<IPropertyHandle> PropHandle = DetailBuilder.GetProperty(Prop->GetFName(), Class);

			if (FName(*TopLevel) != ActiveTab)
			{
				DetailBuilder.HideProperty(PropHandle);
			}
			else
			{
				IDetailPropertyRow* PropRow = DetailBuilder.EditDefaultProperty(PropHandle);

				if (PropRow)
				{
					TSharedPtr<SHorizontalBox> ExtensionBox = SNew(SHorizontalBox);

					// Validation Error Icon if object pointer is null
					if (bShowValidationWarnings && Prop->IsA<FObjectProperty>())
					{
						UObject* ObjValue = nullptr;
						PropHandle->GetValue(ObjValue);
						if (!ObjValue)
						{
							ExtensionBox->AddSlot()
							.AutoWidth()
							.Padding(4, 0, 0, 0)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("⚠️ MISSING!")))
								.ColorAndOpacity(FLinearColor::Red)
								.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
							];
						}
					}

					// Tooltip extension
					FString TooltipStr = Prop->GetToolTipText().ToString();
					if (!TooltipStr.IsEmpty())
					{
						ExtensionBox->AddSlot()
						.AutoWidth()
						.Padding(4, 0, 0, 0)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("ℹ️")))
							.ToolTipText(FText::FromString(TooltipStr))
						];
					}

					if (ExtensionBox->GetChildren()->Num() > 0)
					{
						PropRow->CustomWidget()
						.NameContent()
						[
							PropHandle->CreatePropertyNameWidget()
						]
						.ValueContent()
						[
							PropHandle->CreatePropertyValueWidget()
						]
						.ExtensionContent()
						[
							ExtensionBox.ToSharedRef()
						];
					}
				}
			}
		}
	}
}

void FUniversalDataAssetDetails::CreateDashboard(IDetailLayoutBuilder& DetailBuilder, const TArray<FString>& TopCategories, const FString& ObjectPath, UObject* TargetObject)
{
	bool bIsPremiumEnabled = true;
	GConfig->GetBool(TEXT("DataAssetDesigner"), *ObjectPath, bIsPremiumEnabled, GEditorPerProjectIni);

	TSharedPtr<IPropertyUtilities> PropUtils = DetailBuilder.GetPropertyUtilities();

	IDetailCategoryBuilder& SettingsCategory = DetailBuilder.EditCategory("DesignerSettings", FText::GetEmpty(), ECategoryPriority::Important);
	
	// 1. Top Controls (Toggle & Validate)
	TSharedPtr<SHorizontalBox> TopControlsBox = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked(bIsPremiumEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([ObjectPath, PropUtils](ECheckBoxState NewState) {
				bool bNewEnabled = (NewState == ECheckBoxState::Checked);
				GConfig->SetBool(TEXT("DataAssetDesigner"), *ObjectPath, bNewEnabled, GEditorPerProjectIni);
				if (PropUtils.IsValid()) PropUtils->ForceRefresh();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(" Premium Data Asset"))
				.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
				.ColorAndOpacity(bIsPremiumEnabled ? FLinearColor(0.4f, 0.9f, 0.5f) : FLinearColor(0.5f, 0.5f, 0.5f))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(FText::FromString("Run Validation"))
			.Visibility(bIsPremiumEnabled ? EVisibility::Visible : EVisibility::Hidden)
			.OnClicked_Lambda([this, PropUtils]() -> FReply {
				bShowValidationWarnings = !bShowValidationWarnings;
				if (PropUtils.IsValid()) PropUtils->ForceRefresh();
				return FReply::Handled();
			})
		];

	// 2. Tabs Box
	TSharedPtr<SHorizontalBox> TabsBox = SNew(SHorizontalBox);
	if (bIsPremiumEnabled)
	{
		for (const FString& TabNameStr : TopCategories)
		{
			FName TabFName = FName(*TabNameStr);
			bool bIsActive = (ActiveTab == TabFName);

			TabsBox->AddSlot()
			.AutoWidth()
			.Padding(0, 0, 4, 0)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(bIsActive ? FLinearColor(0.15f, 0.15f, 0.15f, 1.f) : FLinearColor(0.05f, 0.05f, 0.05f, 0.0f))
				.OnClicked_Lambda([this, TabFName, ObjectPath, PropUtils]() -> FReply {
					ActiveTab = TabFName;
					GConfig->SetString(TEXT("DataAssetDesigner"), *(ObjectPath + TEXT("_ActiveTab")), *TabFName.ToString(), GEditorPerProjectIni);
					if (PropUtils.IsValid()) PropUtils->ForceRefresh();
					return FReply::Handled();
				})
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(12.0f, 6.0f, 12.0f, 6.0f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TabNameStr))
						.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
						.ColorAndOpacity(bIsActive ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
						.HeightOverride(3.0f)
						[
							SNew(SColorBlock)
							.Color(bIsActive ? GetColorForCategory(TabNameStr) : FLinearColor::Transparent)
						]
					]
				]
			];
		}
	}

	// 3. Assemble Dashboard
	TSharedPtr<SVerticalBox> DashboardContent = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, bIsPremiumEnabled ? 15 : 0)
		[
			TopControlsBox.ToSharedRef()
		];

	if (bIsPremiumEnabled)
	{
		DashboardContent->AddSlot()
		.AutoHeight()
		[
			TabsBox.ToSharedRef()
		];
	}

	SettingsCategory.AddCustomRow(FText::GetEmpty())
	.WholeRowContent()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10.0f, 10.0f, 10.0f, 0.0f)
		[
			DashboardContent.ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10.0f, 8.0f, 10.0f, 12.0f)
		[
			SNew(SSeparator)
			.Orientation(Orient_Horizontal)
			.Thickness(1.5f)
			.ColorAndOpacity(FLinearColor(0.01f, 0.01f, 0.01f, 0.4f)) // Subtle, professional dark shadow line
		]
	];
}

void FUniversalDataAssetDetails::AddCustomHeader(IDetailLayoutBuilder& DetailBuilder, FName CategoryName, FText Title, FLinearColor Color)
{
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(CategoryName, FText::GetEmpty(), ECategoryPriority::Default);
	
	Category.HeaderContent(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		.Padding(0, 0, 0, 0)
		[
			SNew(STextBlock)
			.Text(Title)
			.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
			.ColorAndOpacity(Color)
			.ShadowColorAndOpacity(FLinearColor::Black)
			.ShadowOffset(FVector2D(1, 1))
		]
	);
}

#undef LOCTEXT_NAMESPACE
