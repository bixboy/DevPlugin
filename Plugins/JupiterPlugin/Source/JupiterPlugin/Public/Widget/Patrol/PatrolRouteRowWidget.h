#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/UnitPatrolComponent.h"
#include "PatrolRouteRowWidget.generated.h"

class UEditableTextBox;
class UCheckBox;
class UCustomButtonWidget;
class UHorizontalBox;
class UButton;
class UComboBoxString;

UCLASS()
class JUPITERPLUGIN_API UPatrolRouteRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	void Setup(UUnitPatrolComponent* InComponent, int32 InRouteIndex);
	void Setup(UUnitPatrolComponent* InComponent, AActor* InTargetUnit);

protected:
	UFUNCTION()
	void OnNameCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void OnWaitTimeCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void OnTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnReverseClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnDeleteClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnArrowsChanged(bool bIsChecked);

	UFUNCTION()
	void OnNumbersChanged(bool bIsChecked);

	UFUNCTION()
	void OnColorButtonClicked(UCustomButtonWidget* Button, int Index);

	void UpdateRoute();
	void RefreshDisplay();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEditableTextBox* NameTextBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEditableTextBox* WaitTimeTextBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UComboBoxString* TypeComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCustomButtonWidget* ReverseButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCustomButtonWidget* DeleteButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* ArrowsCheckBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCheckBox* NumbersCheckBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	class UTextBlock* AssignedUnitsText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UHorizontalBox* ColorContainer;

	// Helper to create color buttons if they don't exist in BP
	void CreateColorButtons();

	UPROPERTY()
	UUnitPatrolComponent* PatrolComponent;

	int32 RouteIndex = -1;
	
	UPROPERTY()
	AActor* TargetUnit = nullptr;

	FPatrolRoute CachedRoute;
};
