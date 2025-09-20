#include "Widget/UnitsSelection/UnitsEntryWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SlectionComponent.h"
#include "Components/TextBlock.h"
#include "Data/UnitsSelectionDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PlayerControllerRts.h"
#include "Units/SoldierRts.h"
#include "Widget/CustomButtonWidget.h"


void UUnitsEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	verify((SelectionComponent = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetPawn()->GetComponentByClass<USelectionComponent>()) != nullptr);

	if (UnitButton)
		UnitButton->OnButtonClicked.AddDynamic(this, &UUnitsEntryWidget::OnUnitSelected);
}

void UUnitsEntryWidget::InitEntry(UUnitsSelectionDataAsset* DataAsset)
{
		FUnitsSelectionData UnitData = DataAsset->UnitSelectionData;
		
		UnitButton->SetButtonTexture(UnitData.UnitImage);
		UnitButton->SetButtonText(UnitData.UnitName);
		UnitClass = UnitData.UnitClass;
}

void UUnitsEntryWidget::OnUnitSelected(UCustomButtonWidget* Button, int Index)
{
	if (!UnitClass && SelectionComponent) return;

	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("ON UNIT SELECTED"));

	SelectionComponent->Server_Reliable_ChangeUnitClass(UnitClass);
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Black, "Unit Selected : " + UnitClass->GetName());
}
