#include "Customizations/VehicleStructCustomizations.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Styling/AppStyle.h"
#include "Data/VehicleData.h"

#define LOCTEXT_NAMESPACE "VehicleStructCustomizations"

// --- SEAT CONFIG CUSTOMIZATION ---

TSharedRef<IPropertyTypeCustomization> FVehicleSeatConfigCustomization::MakeInstance()
{
	return MakeShareable(new FVehicleSeatConfigCustomization);
}

void FVehicleSeatConfigCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(300.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 5, 0)
		[
			SNew(SImage)
			.Image(FAppStyle::Get().GetBrush("Icons.Layout"))
			.ColorAndOpacity(FLinearColor(0.0f, 0.5f, 1.0f))
			.DesiredSizeOverride(FVector2D(14, 14))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([PropertyHandle]() -> FText {
				TSharedPtr<IPropertyHandle> SocketNameProp = PropertyHandle->GetChildHandle("AttachSocketName");
				TSharedPtr<IPropertyHandle> RoleProp = PropertyHandle->GetChildHandle("SeatRole");

				if (SocketNameProp.IsValid() && RoleProp.IsValid())
				{
					FName SocketName;
					SocketNameProp->GetValue(SocketName);
					uint8 RoleValue;
					RoleProp->GetValue(RoleValue);
					
					EVehiclePlaceType Role = static_cast<EVehiclePlaceType>(RoleValue);
					FString RoleStr;
					switch(Role)
					{
						case EVehiclePlaceType::Driver: RoleStr = TEXT("DRIVER"); break;
						case EVehiclePlaceType::Gunner: RoleStr = TEXT("GUNNER"); break;
						case EVehiclePlaceType::Passenger: RoleStr = TEXT("PASSENGER"); break;
						default: RoleStr = TEXT("NONE"); break;
					}

					return FText::FromString(FString::Printf(TEXT("%s [%s]"), *RoleStr, *SocketName.ToString()));
				}
				
				return LOCTEXT("InvalidSeat", "Seat [Loading...]");
			})
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FLinearColor::White)
		]
	];
}

void FVehicleSeatConfigCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	uint32 NumChildren;
	PropertyHandle->GetNumChildren(NumChildren);

	for (uint32 i = 0; i < NumChildren; ++i)
	{
		TSharedPtr<IPropertyHandle> Child = PropertyHandle->GetChildHandle(i);
		if (Child.IsValid())
		{
			ChildBuilder.AddProperty(Child.ToSharedRef());
		}
	}
}

// --- TURRET CONFIG CUSTOMIZATION ---

TSharedRef<IPropertyTypeCustomization> FVehicleTurretConfigCustomization::MakeInstance()
{
	return MakeShareable(new FVehicleTurretConfigCustomization);
}

void FVehicleTurretConfigCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(300.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 5, 0)
		[
			SNew(SImage)
			.Image(FAppStyle::Get().GetBrush("Icons.Refresh"))
			.ColorAndOpacity(FLinearColor(1.0f, 0.5f, 0.0f))
			.DesiredSizeOverride(FVector2D(14, 14))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([PropertyHandle]() -> FText {
				TSharedPtr<IPropertyHandle> SocketNameProp = PropertyHandle->GetChildHandle("AttachSocketName");

				if (SocketNameProp.IsValid())
				{
					FName SocketName;
					SocketNameProp->GetValue(SocketName);
					return FText::FromString(FString::Printf(TEXT("TURRET [%s]"), *SocketName.ToString()));
				}
				
				return LOCTEXT("InvalidTurret", "Turret [Loading...]");
			})
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FLinearColor::White)
		]
	];
}

void FVehicleTurretConfigCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	uint32 NumChildren;
	PropertyHandle->GetNumChildren(NumChildren);

	// Get specific handles for grouping
	TSharedPtr<IPropertyHandle> MaxYawProp = PropertyHandle->GetChildHandle("MaxYaw");
	TSharedPtr<IPropertyHandle> MaxPitchProp = PropertyHandle->GetChildHandle("MaxPitch");

	for (uint32 i = 0; i < NumChildren; ++i)
	{
		TSharedPtr<IPropertyHandle> Child = PropertyHandle->GetChildHandle(i);
		if (!Child.IsValid()) continue;

		// Skip the ones we are going to group
		if (Child->GetProperty()->GetFName() == "MaxYaw" || Child->GetProperty()->GetFName() == "MaxPitch")
		{
			continue;
		}

		ChildBuilder.AddProperty(Child.ToSharedRef());
	}

	// Add the grouped row for Rotation Limits
	if (MaxYawProp.IsValid() && MaxPitchProp.IsValid())
	{
		ChildBuilder.AddCustomRow(LOCTEXT("TurretLimits", "Rotation Limits"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TurretLimitsLabel", "Angle Limits (Yaw/Pitch)"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 5, 0)
				[ SNew(STextBlock).Text(LOCTEXT("YawLabel", "Y:")).Font(IDetailLayoutBuilder::GetDetailFont()) ]
				+ SHorizontalBox::Slot().FillWidth(1.0f) [ MaxYawProp->CreatePropertyValueWidget() ]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0) [ SNew(STextBlock).Text(LOCTEXT("Sep", "/")).Font(IDetailLayoutBuilder::GetDetailFont()) ]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 5, 0)
				[ SNew(STextBlock).Text(LOCTEXT("PitchLabel", "P:")).Font(IDetailLayoutBuilder::GetDetailFont()) ]
				+ SHorizontalBox::Slot().FillWidth(1.0f) [ MaxPitchProp->CreatePropertyValueWidget() ]
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE
