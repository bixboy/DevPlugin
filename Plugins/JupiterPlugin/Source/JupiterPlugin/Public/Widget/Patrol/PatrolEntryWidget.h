#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/UnitPatrolComponent.h"
#include "PatrolEntryWidget.generated.h"

class UTextBlock;
class UImage;
class UTextBlock;
class UImage;
class UCustomButtonWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatrolEntrySelected, FGuid, PatrolID);

/**
 * Compact row for a single patrol entry.
 * Clicking it selects the patrol.
 */
UCLASS(Abstract)
class JUPITERPLUGIN_API UPatrolEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Internal ID to track which patrol this widget represents */
	UPROPERTY(BlueprintReadOnly, Category = "Patrol Entry")
	FGuid PatrolID;

	UPROPERTY(BlueprintAssignable, Category = "Patrol Entry")
	FOnPatrolEntrySelected OnEntrySelected;

	// --- Visual Components ---

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_RouteName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_UnitCount;

	UPROPERTY(meta = (BindWidget))
	UImage* Image_RouteColor;
	
	/** Main button to select the row */
	UPROPERTY(meta = (BindWidget))
	UCustomButtonWidget* Btn_Select;

	/** Background or border to show selection state */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* Image_SelectionHighlight;

	// --- Logic ---

	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable, Category = "Patrol Entry")
	void Setup(const FPatrolRoute& Route);

	UFUNCTION(BlueprintCallable, Category = "Patrol Entry")
	void SetIsSelected(bool bIsSelected);

protected:
	UFUNCTION()
	void OnSelectClicked(UCustomButtonWidget* Button, int Index);

	// Style settings
	UPROPERTY(EditDefaultsOnly, Category = "Style")
	FLinearColor SelectedColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.2f);

	UPROPERTY(EditDefaultsOnly, Category = "Style")
	FLinearColor NormalColor = FLinearColor::Transparent;
};
