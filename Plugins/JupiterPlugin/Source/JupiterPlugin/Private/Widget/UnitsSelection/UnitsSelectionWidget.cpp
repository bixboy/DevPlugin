#include "Widget/UnitsSelection/UnitsSelectionWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/UnitSpawnComponent.h"
#include "Components/WrapBox.h"
#include "Data/UnitsSelectionDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Widget/CustomButtonWidget.h"
#include "Widget/UnitsSelection/UnitsEntryWidget.h"


void UUnitsSelectionWidget::NativeOnInitialized()
{
        Super::NativeOnInitialized();

        if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
        {
                if (APawn* PlayerPawn = PlayerController->GetPawn())
                        SpawnComponent = PlayerPawn->FindComponentByClass<UUnitSpawnComponent>();
        }

        if (SpawnComponent)
        {
                CachedSpawnCount = FMath::Max(1, SpawnComponent->GetUnitsPerSpawn());
                SpawnComponent->OnSpawnCountChanged.AddDynamic(this, &UUnitsSelectionWidget::HandleSpawnCountChanged);
        }
        else
        {
                CachedSpawnCount = 1;
        }

        if (SpawnCountTextBox)
        {
                SpawnCountTextBox->OnTextCommitted.AddDynamic(this, &UUnitsSelectionWidget::OnSpawnCountTextCommitted);
        }

        if (Btn_IncreaseSpawnCount)
        {
                Btn_IncreaseSpawnCount->OnButtonClicked.AddDynamic(this, &UUnitsSelectionWidget::OnIncreaseSpawnCount);
        }

        if (Btn_DecreaseSpawnCount)
        {
                Btn_DecreaseSpawnCount->OnButtonClicked.AddDynamic(this, &UUnitsSelectionWidget::OnDecreaseSpawnCount);
        }

        if (Btn_ShowUnitsSelection)
                Btn_ShowUnitsSelection->OnPressed.AddDynamic(this, &UUnitsSelectionWidget::OnShowUnitSelectionPressed);

        RefreshSpawnCountDisplay();
        OnShowUnitSelectionPressed();
        SetupUnitsList();
}

void UUnitsSelectionWidget::NativeDestruct()
{
        if (SpawnComponent)
        {
                SpawnComponent->OnSpawnCountChanged.RemoveDynamic(this, &UUnitsSelectionWidget::HandleSpawnCountChanged);
        }

        if (SpawnCountTextBox)
        {
                SpawnCountTextBox->OnTextCommitted.RemoveDynamic(this, &UUnitsSelectionWidget::OnSpawnCountTextCommitted);
        }

        if (Btn_IncreaseSpawnCount)
        {
                Btn_IncreaseSpawnCount->OnButtonClicked.RemoveDynamic(this, &UUnitsSelectionWidget::OnIncreaseSpawnCount);
        }

        if (Btn_DecreaseSpawnCount)
        {
                Btn_DecreaseSpawnCount->OnButtonClicked.RemoveDynamic(this, &UUnitsSelectionWidget::OnDecreaseSpawnCount);
        }

        if (Btn_ShowUnitsSelection)
        {
                Btn_ShowUnitsSelection->OnPressed.RemoveDynamic(this, &UUnitsSelectionWidget::OnShowUnitSelectionPressed);
        }

        Super::NativeDestruct();
}

void UUnitsSelectionWidget::SetupUnitsList()
{
        if (!WrapBox) return;

        WrapBox->ClearChildren();

	for (UUnitsSelectionDataAsset* Data : UnitsSelectionDataAssets)
	{
		if (Data)
		{
			UUnitsEntryWidget* UnitWidget = CreateWidget<UUnitsEntryWidget>(GetWorld(), UnitsEntryClass);
			if (UnitWidget)
			{
				UnitWidget->InitEntry(Data);
				
				WrapBox->AddChild(UnitWidget);
				EntryList.Add(UnitWidget);
				
				UnitWidget->UnitButton->OnButtonClicked.AddDynamic(this, &UUnitsSelectionWidget::OnUnitSelected);
			}
		}
	}
}

void UUnitsSelectionWidget::OnShowUnitSelectionPressed()
{
	if (ListBorder->GetVisibility() == ESlateVisibility::Visible)
	{
		ListBorder->SetVisibility(ESlateVisibility::Collapsed);	
	}
	else
	{
		ListBorder->SetVisibility(ESlateVisibility::Visible);	
	}
}

void UUnitsSelectionWidget::OnUnitSelected(UCustomButtonWidget* Button, int Index)
{
        for (UUnitsEntryWidget* EntryWidget : EntryList)
        {
                if (EntryWidget)
                        EntryWidget->UnitButton->ToggleButtonIsSelected(false);
        }

        if (Button)
                Button->ToggleButtonIsSelected(true);
}

void UUnitsSelectionWidget::OnSpawnCountTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
        int32 ParsedValue = CachedSpawnCount;
        const FString TextString = Text.ToString();
        
        if (!TextString.IsEmpty())
                ParsedValue = FCString::Atoi(*TextString);

        ApplySpawnCount(ParsedValue);
}

void UUnitsSelectionWidget::OnIncreaseSpawnCount(UCustomButtonWidget* /*Button*/, int /*Index*/)
{
        ApplySpawnCount(CachedSpawnCount + 1);
}

void UUnitsSelectionWidget::OnDecreaseSpawnCount(UCustomButtonWidget* /*Button*/, int /*Index*/)
{
        ApplySpawnCount(CachedSpawnCount - 1);
}

void UUnitsSelectionWidget::HandleSpawnCountChanged(int32 NewCount)
{
        CachedSpawnCount = FMath::Max(1, NewCount);
        RefreshSpawnCountDisplay();
}

void UUnitsSelectionWidget::ApplySpawnCount(int32 NewCount)
{
        const int32 ClampedCount = FMath::Clamp(NewCount, 1, MaxSpawnCount);

        if (SpawnComponent)
                SpawnComponent->SetUnitsPerSpawn(ClampedCount);

        CachedSpawnCount = ClampedCount;
        RefreshSpawnCountDisplay();
}

void UUnitsSelectionWidget::RefreshSpawnCountDisplay() const
{
        if (SpawnCountTextBox)
                SpawnCountTextBox->SetText(FText::AsNumber(CachedSpawnCount));
}
