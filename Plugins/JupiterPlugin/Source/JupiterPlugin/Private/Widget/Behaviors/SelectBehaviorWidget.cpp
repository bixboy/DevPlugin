#include "Widget/Behaviors/SelectBehaviorWidget.h"

#include "Components/ComboBoxString.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Units/SoldierRts.h"

void USelectBehaviorWidget::NativeOnInitialized()
{
        Super::NativeOnInitialized();

        if (APawn* OwnerPawn = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetPawn())
        {
                SelectionComponent = OwnerPawn->FindComponentByClass<UUnitSelectionComponent>();
                OrderComponent = OwnerPawn->FindComponentByClass<UUnitOrderComponent>();
        }

        InitializeBehaviorOptions();

        if (SelectionComponent)
        {
                SelectionComponent->OnSelectedUpdate.AddDynamic(this, &USelectBehaviorWidget::OnNewUnitSelected);
        }
}

void USelectBehaviorWidget::OnBehaviorSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
        if (bIsUpdatingSelection)
        {
                return;
        }

        ApplyBehaviorSelection(SelectedItem);
}

void USelectBehaviorWidget::OnNewUnitSelected()
{
        if (!SelectionComponent || !BehaviorDropdown)
        {
                return;
        }

        bIsUpdatingSelection = true;
        BehaviorDropdown->ClearSelection();

        if (SelectionComponent->GetSelectedActors().Num() == 1)
        {
                if (ASoldierRts* Unit = Cast<ASoldierRts>(SelectionComponent->GetSelectedActors()[0]))
                {
                        UpdateSelectedBehavior(Unit->GetCombatBehavior());
                }
        }

        bIsUpdatingSelection = false;
}

void USelectBehaviorWidget::InitializeBehaviorOptions()
{
        if (!BehaviorDropdown)
        {
                return;
        }

        BehaviorDropdown->ClearOptions();
        OptionToBehavior.Reset();
        BehaviorToOption.Reset();

        if (const UEnum* EnumPtr = StaticEnum<ECombatBehavior>())
        {
                for (int32 EnumIndex = 0; EnumIndex < EnumPtr->NumEnums(); ++EnumIndex)
                {
                        if (EnumPtr->HasMetaData(TEXT("Hidden"), EnumIndex))
                        {
                                continue;
                        }

                        const FString EnumName = EnumPtr->GetNameStringByIndex(EnumIndex);
                        if (EnumName.Contains(TEXT("MAX")))
                        {
                                continue;
                        }

                        const ECombatBehavior Behavior = static_cast<ECombatBehavior>(EnumPtr->GetValueByIndex(EnumIndex));
                        const FString DisplayName = EnumPtr->GetDisplayNameTextByIndex(EnumIndex).ToString();

                        BehaviorDropdown->AddOption(DisplayName);
                        OptionToBehavior.Add(DisplayName, Behavior);
                        BehaviorToOption.Add(Behavior, DisplayName);
                }

                if (BehaviorDropdown->GetOptionCount() > 0)
                {
                        const FString& DefaultOption = BehaviorDropdown->GetOptionAtIndex(0);
                        bIsUpdatingSelection = true;
                        BehaviorDropdown->SetSelectedOption(DefaultOption);
                        bIsUpdatingSelection = false;

                        ApplyBehaviorSelection(DefaultOption);
                }
        }

        BehaviorDropdown->OnSelectionChanged.AddDynamic(this, &USelectBehaviorWidget::OnBehaviorSelectionChanged);
}

void USelectBehaviorWidget::ApplyBehaviorSelection(const FString& SelectedOption)
{
        if (!OrderComponent)
        {
                return;
        }

        if (const ECombatBehavior* Behavior = OptionToBehavior.Find(SelectedOption))
        {
                OrderComponent->UpdateBehavior(*Behavior);
        }
}

void USelectBehaviorWidget::UpdateSelectedBehavior(ECombatBehavior Behavior)
{
        if (!BehaviorDropdown)
        {
                return;
        }

        if (const FString* Option = BehaviorToOption.Find(Behavior))
        {
                BehaviorDropdown->SetSelectedOption(*Option);
        }
}
