// #include "Widget/Formations/FormationSelectorWidget.h"
// #include "Player/PlayerControllerRts.h"
// #include "Components/SlectionComponent.h"
// #include "Components/Slider.h"
// #include "Kismet/GameplayStatics.h"
// #include "Widget/Formations/FormationButtonWidget.h"
//
// void UFormationSelectorWidget::NativeOnInitialized()
// {
// 	Super::NativeOnInitialized();
//
// 	verify((SelectionComponent = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetPawn()->GetComponentByClass<USelectionComponent>()) != nullptr);
//
// 	if(!SelectionComponent)
// 	{
// 		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("SELECTION COMPONENT NULL"));
// 	}
// 	
// 	if (LineButton && ColumnButton && WedgeButton && BlobButton && SquareButton)
// 	{
// 		OnFormationButtonClicked(SquareButton->Button, 0);
// 		LineButton->Button->OnButtonClicked.AddDynamic(this, &UFormationSelectorWidget::OnFormationButtonClicked);
// 		ColumnButton->Button->OnButtonClicked.AddDynamic(this, &UFormationSelectorWidget::OnFormationButtonClicked);
// 		WedgeButton->Button->OnButtonClicked.AddDynamic(this, &UFormationSelectorWidget::OnFormationButtonClicked);
// 		BlobButton->Button->OnButtonClicked.AddDynamic(this, &UFormationSelectorWidget::OnFormationButtonClicked);
// 		SquareButton->Button->OnButtonClicked.AddDynamic(this, &UFormationSelectorWidget::OnFormationButtonClicked);
// 	}
//
// 	if (SpacingSlider)
// 	{
// 		SpacingSlider->OnValueChanged.AddDynamic(this, &UFormationSelectorWidget::OnSpacingSliderValueChanged);
// 		OnSpacingSliderValueChanged(SpacingSlider->GetValue());
// 	}
// }
//
// void UFormationSelectorWidget::OnFormationButtonClicked(UCustomButtonWidget* Button, int Index)
// {
// 	if (SelectionComponent)
// 	{
// 		SelectionComponent->UpdateFormation(static_cast<EFormation>(Index));
//
// 		LineButton->Button->ToggleButtonIsSelected(false);
// 		ColumnButton->Button->ToggleButtonIsSelected(false);
// 		WedgeButton->Button->ToggleButtonIsSelected(false);
// 		BlobButton->Button->ToggleButtonIsSelected(false);
// 		SquareButton->Button->ToggleButtonIsSelected(false);
//
// 		Button->ToggleButtonIsSelected(true);
// 	}else
// 	{
// 		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("NO SELECTION COMPONENT"));
// 	}
// }
//
// void UFormationSelectorWidget::OnSpacingSliderValueChanged(const float Value)
// {
// 	GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, TEXT( "SpacingSliderValueChanged"));
//
// 	if (SelectionComponent)
// 	{
// 		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green, FString::Printf(TEXT("Spacing Value: %f"), Value));
//
// 		SelectionComponent->UpdateSpacing(Value);
// 	}else
// 	{
// 		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("NO SELECTION COMPONENT"));
// 	}
// }



#include "Widget/Formations/FormationSelectorWidget.h"
#include "Components/SlectionComponent.h"
#include "Components/Slider.h"
#include "Kismet/GameplayStatics.h"
#include "Widget/Formations/FormationButtonWidget.h"

void UFormationSelectorWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	verify((SelectionComponent = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetPawn()->GetComponentByClass<USelectionComponent>()) != nullptr);

	if(!SelectionComponent)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("SELECTION COMPONENT NULL"));
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
	if (SelectionComponent)
	{
		SelectionComponent->UpdateFormation(static_cast<EFormation>(Index));

		LineButton->Button->ToggleButtonIsSelected(false);
		ColumnButton->Button->ToggleButtonIsSelected(false);
		WedgeButton->Button->ToggleButtonIsSelected(false);
		BlobButton->Button->ToggleButtonIsSelected(false);
		SquareButton->Button->ToggleButtonIsSelected(false);

		Button->ToggleButtonIsSelected(true);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("NO SELECTION COMPONENT"));
	}
}

void UFormationSelectorWidget::OnSpacingSliderValueChanged(const float Value)
{
	if (SelectionComponent)
	{
		SelectionComponent->UpdateSpacing(Value);
	}else
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("NO SELECTION COMPONENT"));
	}
}
