#include "Widget/UnitsSelection/UnitsEntryWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/UnitSpawnComponent.h"
#include "Components/TextBlock.h"
#include "Data/UnitsSelectionDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PlayerControllerRts.h"
#include "Units/SoldierRts.h"
#include "Widget/CustomButtonWidget.h"


void UUnitsEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

    if (APawn* OwnerPawn = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetPawn())
                SpawnComponent = OwnerPawn->FindComponentByClass<UUnitSpawnComponent>();

        if (UnitButton)
                UnitButton->OnButtonClicked.AddDynamic(this, &UUnitsEntryWidget::OnUnitSelected);
}

void UUnitsEntryWidget::InitEntry(UUnitsSelectionDataAsset* DataAsset)
{
        FUnitsSelectionData UnitData = DataAsset->UnitSelectionData;

        CachedUnitName = UnitData.UnitName;
        UnitTags = UnitData.UnitTags;

        if (UnitButton)
        {
                UnitButton->SetButtonTexture(UnitData.UnitImage);
                UnitButton->SetButtonText(CachedUnitName);
        }
        UnitClass = UnitData.UnitClass;
}

void UUnitsEntryWidget::OnUnitSelected(UCustomButtonWidget* Button, int Index)
{
    if (!UnitClass || !SpawnComponent)
		return;

    SpawnComponent->SetUnitToSpawn(UnitClass);
        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Black, "Unit Selected : " + UnitClass->GetName());
}

bool UUnitsEntryWidget::MatchesSearch(const FString& SearchLower) const
{
        if (SearchLower.IsEmpty())
        {
                return true;
        }

        FString NameLower = CachedUnitName.ToString();
        NameLower.ToLowerInline();

        return NameLower.Contains(SearchLower);
}

bool UUnitsEntryWidget::HasTag(FName Tag) const
{
        if (Tag.IsNone())
        {
                return true;
        }

        return UnitTags.Contains(Tag);
}
