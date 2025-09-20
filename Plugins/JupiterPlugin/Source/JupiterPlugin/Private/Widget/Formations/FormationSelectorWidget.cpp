#include "Widget/Formations/FormationSelectorWidget.h"
#include "Components/UnitFormationComponent.h"
#include "Components/Slider.h"
#include "Kismet/GameplayStatics.h"
#include "Widget/Formations/FormationButtonWidget.h"

void UFormationSelectorWidget::NativeOnInitialized()
{
        Super::NativeOnInitialized();

        if (APawn* OwnerPawn = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetPawn())
        {
                FormationComponent = OwnerPawn->FindComponentByClass<UUnitFormationComponent>();
        }

        if(!FormationComponent)
        {
                GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("FORMATION COMPONENT NULL"));
        }

        if (LineButton && ColumnButton && WedgeButton && BlobButton && SquareButton)
        {
                OnFormationButtonClicked(SquareButton->Button, 0);
                LineButton->Button->OnButtonClicked.AddDynamic(this, &UFormationSelectorWidget::OnFormationButtonClicked);
                ColumnButton->Button->OnButtonClicked.AddDynamic(this, &UFormationSelectorWidget::OnFormationButtonClicked);
                WedgeButton->Button->OnButtonClicked.AddDynamic(this, &UFormationSelectorWidget::OnFormationButtonClicked);
                BlobButton->Button->OnButtonClicked.AddDynamic(this, &UFormationSelectorWidget::OnFormationButtonClicked);
                SquareButton->Button->OnButtonClicked.AddDynamic(this, &UFormationSelectorWidget::OnFormationButtonClicked);
        }

        if (SpacingSlider)
        {
                SpacingSlider->OnValueChanged.AddDynamic(this, &UFormationSelectorWidget::OnSpacingSliderValueChanged);
                OnSpacingSliderValueChanged(SpacingSlider->GetValue());
        }
}

void UFormationSelectorWidget::OnFormationButtonClicked(UCustomButtonWidget* Button, int Index)
{
        if (FormationComponent)
        {
                FormationComponent->SetFormation(static_cast<EFormation>(Index));

                LineButton->Button->ToggleButtonIsSelected(false);
                ColumnButton->Button->ToggleButtonIsSelected(false);
                WedgeButton->Button->ToggleButtonIsSelected(false);
                BlobButton->Button->ToggleButtonIsSelected(false);
                SquareButton->Button->ToggleButtonIsSelected(false);

                Button->ToggleButtonIsSelected(true);
        }
        else
        {
                GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("NO FORMATION COMPONENT"));
        }
}

void UFormationSelectorWidget::OnSpacingSliderValueChanged(const float Value)
{
        if (FormationComponent)
        {
                FormationComponent->SetFormationSpacing(Value);
        }
        else
        {
                GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("NO FORMATION COMPONENT"));
        }
}
