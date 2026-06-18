#include "Customizations/VehicleDataAssetDetails.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/AppStyle.h"
#include "Data/VehicleData.h"

#define LOCTEXT_NAMESPACE "VehicleDataAssetDetails"

TSharedRef<IDetailCustomization> FVehicleDataAssetDetails::MakeInstance()
{
	return MakeShareable(new FVehicleDataAssetDetails);
}

void FVehicleDataAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// 1. Reference the Categories from Header (No numbers as per latest header update)
	IDetailCategoryBuilder& CoreCat = DetailBuilder.EditCategory("Core", LOCTEXT("Core", "Core Configuration"), ECategoryPriority::Important);
	IDetailCategoryBuilder& SetupCat = DetailBuilder.EditCategory("Setup", LOCTEXT("Setup", "Setup"), ECategoryPriority::Important);
	IDetailCategoryBuilder& MechanicsCat = DetailBuilder.EditCategory("Mechanics", LOCTEXT("Mechanics", "Mechanics"), ECategoryPriority::Important);
	IDetailCategoryBuilder& CameraCat = DetailBuilder.EditCategory("Camera", LOCTEXT("Camera", "Camera"), ECategoryPriority::Default);
	IDetailCategoryBuilder& NetworkCat = DetailBuilder.EditCategory("Network", LOCTEXT("Network", "Network & Prediction"), ECategoryPriority::Default);
	
	// 2. Custom Header Row
	CoreCat.AddCustomRow(LOCTEXT("StellarHeader", "Stellar"))
	.WholeRowContent()
	[
		SNew(SBox)
		.Padding(FMargin(0, 5, 0, 15))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 10, 0)
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Layout"))
				.ColorAndOpacity(FLinearColor(0.0f, 0.5f, 1.0f))
				.DesiredSizeOverride(FVector2D(24, 24))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("StellarTitle", "STELLAR VEHICLE ARCHITECT"))
				.Font(FAppStyle::Get().GetFontStyle("DetailsView.CategoryFontStyle"))
				.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			]
		]
	];

	// 3. Properties mapping
	CoreCat.AddProperty(DetailBuilder.GetProperty("VehicleClass"));
	CoreCat.AddProperty(DetailBuilder.GetProperty("ChassisType"));
	CoreCat.AddProperty(DetailBuilder.GetProperty("ChassisStaticMesh"));
	CoreCat.AddProperty(DetailBuilder.GetProperty("ChassisSkeletalMesh"));
	CoreCat.AddProperty(DetailBuilder.GetProperty("AnimClass"));
	CoreCat.AddProperty(DetailBuilder.GetProperty("ChassisRotationOffset"));

	SetupCat.AddProperty(DetailBuilder.GetProperty("Seats"));
	SetupCat.AddProperty(DetailBuilder.GetProperty("Turrets"));
	SetupCat.AddProperty(DetailBuilder.GetProperty("OutOfVehicleOffset")); // Moved here

	MechanicsCat.AddProperty(DetailBuilder.GetProperty("Movement"));
	MechanicsCat.AddProperty(DetailBuilder.GetProperty("Audio"));
	MechanicsCat.AddProperty(DetailBuilder.GetProperty("Input"));
	MechanicsCat.AddProperty(DetailBuilder.GetProperty("Sensitivity"));

	NetworkCat.AddProperty(DetailBuilder.GetProperty("Network"));

	// --- 4. Camera ---
	CameraCat.AddProperty(DetailBuilder.GetProperty("CameraSocketName"));
	CameraCat.AddProperty(DetailBuilder.GetProperty("CameraDistance"));
	CameraCat.AddProperty(DetailBuilder.GetProperty("CameraLagSpeed"));

	// Pitch Limits Group
	CameraCat.AddCustomRow(LOCTEXT("PitchLimits", "Pitch Limits"))
	.NameContent()
	[
		SNew(STextBlock).Text(LOCTEXT("PitchLimitsLabel", "Pitch Limits (Min/Max)")).Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(250.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f) [ DetailBuilder.GetProperty("MinPitchClamp")->CreatePropertyValueWidget() ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0) [ SNew(STextBlock).Text(LOCTEXT("To", "/")).Font(IDetailLayoutBuilder::GetDetailFont()) ]
		+ SHorizontalBox::Slot().FillWidth(1.0f) [ DetailBuilder.GetProperty("MaxPitchClamp")->CreatePropertyValueWidget() ]
	];

	// FOV Range Group
	CameraCat.AddCustomRow(LOCTEXT("FOVRange", "FOV Range"))
	.NameContent()
	[
		SNew(STextBlock).Text(LOCTEXT("FOVRangeLabel", "FOV Range (Min/Max)")).Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(250.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f) [ DetailBuilder.GetProperty("MinFOV")->CreatePropertyValueWidget() ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0) [ SNew(STextBlock).Text(LOCTEXT("To", "/")).Font(IDetailLayoutBuilder::GetDetailFont()) ]
		+ SHorizontalBox::Slot().FillWidth(1.0f) [ DetailBuilder.GetProperty("MaxFOV")->CreatePropertyValueWidget() ]
	];

	// Hide
	DetailBuilder.HideProperty("MinPitchClamp");
	DetailBuilder.HideProperty("MaxPitchClamp");
	DetailBuilder.HideProperty("MinFOV");
	DetailBuilder.HideProperty("MaxFOV");
}

void FVehicleDataAssetDetails::AddCustomHeader(IDetailLayoutBuilder& DetailBuilder, FName CategoryName, FText Title, FLinearColor Color, FName IconName)
{
}

#undef LOCTEXT_NAMESPACE
