#include "UI/Editor/Widgets/Spawn/UnitSpawnFormationWidget.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"


void UUnitSpawnFormationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (FormationDropdown)
    {
        UE_LOG(LogTemp, Warning, TEXT("UnitSpawnFormationWidget::NativeOnInitialized - FormationDropdown FOUND"));
        FormationDropdown->OnSelectionChanged.AddDynamic(this, &UUnitSpawnFormationWidget::OnFormationChanged);
        FormationDropdown->OnGenerateWidgetEvent.BindDynamic(this, &UUnitSpawnFormationWidget::HandleGenerateWidget);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UnitSpawnFormationWidget::NativeOnInitialized - FormationDropdown MISSING"));
    }

    InitializeFormationOptions();
}

void UUnitSpawnFormationWidget::NativeDestruct()
{
    if (SpawnComponent.IsValid())
    {
        SpawnComponent->OnSpawnFormationChanged.RemoveDynamic(this, &UUnitSpawnFormationWidget::UpdateSelectionFromComponent);
    }
    
    Super::NativeDestruct();
}

void UUnitSpawnFormationWidget::SetupWithComponent(UUnitSpawnComponent* InSpawnComponent)
{
    UE_LOG(LogTemp, Warning, TEXT("UnitSpawnFormationWidget::SetupWithComponent - InSpawnComponent: %s"), InSpawnComponent ? *InSpawnComponent->GetName() : TEXT("NULL"));
    
    if (SpawnComponent.IsValid())
    {
        SpawnComponent->OnSpawnFormationChanged.RemoveDynamic(this, &UUnitSpawnFormationWidget::UpdateSelectionFromComponent);
    }

    SpawnComponent = InSpawnComponent;
    UpdateSelectionFromComponent();

    if (SpawnComponent.IsValid())
    {
        SpawnComponent->OnSpawnFormationChanged.AddUniqueDynamic(this, &UUnitSpawnFormationWidget::UpdateSelectionFromComponent);
    }
}

void UUnitSpawnFormationWidget::InitializeFormationOptions()
{
    if (!FormationDropdown)
    	return;

    FormationDropdown->ClearOptions();
    OptionToFormation.Reset();
    FormationToOption.Reset();

    const UEnum* EnumPtr = StaticEnum<ESpawnFormation>();
    if (!EnumPtr)
    	return;

    for (int32 i = 0; i < EnumPtr->NumEnums(); ++i)
    {
        if (EnumPtr->HasMetaData(TEXT("Hidden"), i))
        	continue;

        if (EnumPtr->GetNameStringByIndex(i).Contains(TEXT("MAX")))
        	continue;

        const ESpawnFormation FormationValue = static_cast<ESpawnFormation>(EnumPtr->GetValueByIndex(i));
        const FString DisplayName = EnumPtr->GetDisplayNameTextByIndex(i).ToString();

        FormationDropdown->AddOption(DisplayName);
        OptionToFormation.Add(DisplayName, FormationValue);
        FormationToOption.Add(FormationValue, DisplayName);
    }

	if (FormationDropdown->GetOptionCount() > 0)
	{
		UpdateMainDisplay(FormationDropdown->GetSelectedOption());
	}
}

void UUnitSpawnFormationWidget::UpdateSelectionFromComponent(ESpawnFormation /*NewFormation*/)
{
    if (!FormationDropdown || !SpawnComponent.IsValid())
    	return;

    const ESpawnFormation CurrentFormation = SpawnComponent->GetSpawnFormation();

    if (const FString* FoundLabel = FormationToOption.Find(CurrentFormation))
    {
        bUpdatingSelection = true;
        FormationDropdown->SetSelectedOption(*FoundLabel);
    	UpdateMainDisplay(*FoundLabel);
        bUpdatingSelection = false;
    }
}

void UUnitSpawnFormationWidget::UpdateSelectionFromComponent()
{
    UpdateSelectionFromComponent(ESpawnFormation::Square); 
}

void UUnitSpawnFormationWidget::OnFormationChanged(FString SelectedItem, ESelectInfo::Type /*SelectionType*/)
{
	UpdateMainDisplay(SelectedItem);
	
    if (bUpdatingSelection || !SpawnComponent.IsValid())
    	return;

    if (const ESpawnFormation* FoundFormation = OptionToFormation.Find(SelectedItem))
    {
        SpawnComponent->SetSpawnFormation(*FoundFormation);
    }
}

UWidget* UUnitSpawnFormationWidget::HandleGenerateWidget(FString Item)
{
    if (!WidgetTree)
    {
        return nullptr;
    }

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    
	const ESpawnFormation* FormationPtr = OptionToFormation.Find(Item);
	UTexture2D* IconTexture = (FormationPtr && FormationIcons.Contains(*FormationPtr)) ? *FormationIcons.Find(*FormationPtr) : nullptr;

	if (IconTexture)
	{
		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
		IconBox->SetWidthOverride(24.f);
		IconBox->SetHeightOverride(24.f);

		UImage* IconImage = WidgetTree->ConstructWidget<UImage>();
		IconImage->SetBrushFromTexture(IconTexture);
		IconImage->SetDesiredSizeOverride(FVector2D(24.f, 24.f));
        
		IconBox->AddChild(IconImage);

		UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconBox);
		if (IconSlot)
		{
			IconSlot->SetVerticalAlignment(VAlign_Center);
			IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
			IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	UTextBlock* TextLabel = WidgetTree->ConstructWidget<UTextBlock>();
	TextLabel->SetText(FText::FromString(Item));
	TextLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White)); 
    
	UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextLabel);
	if (TextSlot)
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	return Row;
}

void UUnitSpawnFormationWidget::UpdateMainDisplay(const FString& SelectedItem)
{
	const ESpawnFormation* FormationPtr = OptionToFormation.Find(SelectedItem);
    
	if (SelectedLabel)
		SelectedLabel->SetText(FText::FromString(SelectedItem));

	if (SelectedIcon)
	{
		UTexture2D* IconTexture = (FormationPtr && FormationIcons.Contains(*FormationPtr)) ? *FormationIcons.Find(*FormationPtr) : nullptr;
        
		if (IsValid(IconTexture))
		{
			SelectedIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
			SelectedIcon->SetBrushFromTexture(IconTexture);
		}
		else
		{
			SelectedIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}