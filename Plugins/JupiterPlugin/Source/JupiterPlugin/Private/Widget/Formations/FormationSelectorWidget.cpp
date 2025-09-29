#include "Widget/Formations/FormationSelectorWidget.h"

#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/UnitFormationComponent.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

void UFormationSelectorWidget::NativeOnInitialized()
{
        Super::NativeOnInitialized();

        if (APawn* OwnerPawn = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetPawn())
        {
                FormationComponent = OwnerPawn->FindComponentByClass<UUnitFormationComponent>();
        }

        if (!FormationComponent)
        {
                GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("FORMATION COMPONENT NULL"));
        }
        else
        {
                FormationComponent->OnFormationStateChanged.AddDynamic(this, &UFormationSelectorWidget::OnFormationStateChanged);
        }

        InitializeFormationOptions();
        UpdateSpacingFromComponent();

        if (SpacingSlider)
        {
                SpacingSlider->OnValueChanged.AddDynamic(this, &UFormationSelectorWidget::OnSpacingSliderValueChanged);
        }
}

void UFormationSelectorWidget::OnFormationSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
        if (bIsUpdatingSelection)
                return;

        ApplyFormationSelection(SelectedItem);
}

void UFormationSelectorWidget::OnSpacingSliderValueChanged(const float Value)
{
        if (bIsUpdatingSpacing)
        {
                return;
        }

        if (FormationComponent)
        {
                FormationComponent->SetFormationSpacing(Value);
        }
        else
        {
                GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("NO FORMATION COMPONENT"));
        }
}

void UFormationSelectorWidget::OnFormationStateChanged()
{
        UpdateSelectedFormation(FormationComponent ? FormationComponent->GetFormation() : EFormation::Line);
        UpdateSpacingFromComponent();
}

void UFormationSelectorWidget::InitializeFormationOptions()
{
        if (!FormationDropdown)
        {
                return;
        }

        FormationDropdown->ClearOptions();
        OptionToFormation.Reset();
        FormationToOption.Reset();

        if (const UEnum* EnumPtr = StaticEnum<EFormation>())
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

                        const EFormation Formation = static_cast<EFormation>(EnumPtr->GetValueByIndex(EnumIndex));
                        const FString DisplayName = EnumPtr->GetDisplayNameTextByIndex(EnumIndex).ToString();

                        FormationDropdown->AddOption(DisplayName);
                        OptionToFormation.Add(DisplayName, Formation);
                        FormationToOption.Add(Formation, DisplayName);
                }
        }

        FormationDropdown->OnSelectionChanged.AddDynamic(this, &UFormationSelectorWidget::OnFormationSelectionChanged);

        if (FormationComponent)
        {
                UpdateSelectedFormation(FormationComponent->GetFormation());
        }
        else if (FormationDropdown->GetOptionCount() > 0)
        {
                const FString& DefaultOption = FormationDropdown->GetOptionAtIndex(0);
                bIsUpdatingSelection = true;
                FormationDropdown->SetSelectedOption(DefaultOption);
                bIsUpdatingSelection = false;

                ApplyFormationSelection(DefaultOption);
        }
}

void UFormationSelectorWidget::ApplyFormationSelection(const FString& SelectedOption)
{
        if (!FormationComponent)
        {
                GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("NO FORMATION COMPONENT"));
                return;
        }

        if (const EFormation* Formation = OptionToFormation.Find(SelectedOption))
                FormationComponent->SetFormation(*Formation);
}

void UFormationSelectorWidget::UpdateSelectedFormation(EFormation Formation)
{
        if (!FormationDropdown)
                return;

        if (const FString* Option = FormationToOption.Find(Formation))
        {
                bIsUpdatingSelection = true;
                FormationDropdown->SetSelectedOption(*Option);
                bIsUpdatingSelection = false;
        }
}

void UFormationSelectorWidget::UpdateSpacingFromComponent()
{
        if (!SpacingSlider || !FormationComponent)
                return;

        const float ComponentSpacing = FormationComponent->GetFormationSpacing();
        if (!FMath::IsNearlyEqual(ComponentSpacing, SpacingSlider->GetValue()))
        {
                bIsUpdatingSpacing = true;
                SpacingSlider->SetValue(ComponentSpacing);
                bIsUpdatingSpacing = false;
        }
}
