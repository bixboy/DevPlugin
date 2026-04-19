#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Patrol/UnitPatrolComponent.h"
#include "PatrolEntryWidget.generated.h"

class UTextBlock;
class UImage;
class UCustomButtonWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPatrolEntrySelected, int32, Index, UPatrolEntryWidget*, EntryWidget);


UCLASS(Abstract)
class JUPITERPLUGIN_API UPatrolEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Patrol Entry")
	FGuid PatrolID;

	UPROPERTY(BlueprintReadOnly, Category = "Patrol Entry")
	int32 EntryIndex = -1;

	UPROPERTY(BlueprintAssignable, Category = "Patrol Entry")
	FOnPatrolEntrySelected OnEntrySelected;

	// --- Visual Components ---

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* Text_RouteName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* Text_UnitCount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* Text_Distance;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* Image_RouteColor;
	
	UPROPERTY(meta = (BindWidget))
	UCustomButtonWidget* Btn_Select;

	UPROPERTY(meta = (BindWidget))
	UImage* SelectionBackground;

	// --- Logic ---

	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable, Category = "Patrol Entry")
	void Setup(const FPatrolRoute& Route, int32 Index, float Distance = -1.0f);

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

	// Icons
	UPROPERTY(EditDefaultsOnly, Category = "Style|Icons")
	UTexture2D* Icon_Loop;

	UPROPERTY(EditDefaultsOnly, Category = "Style|Icons")
	UTexture2D* Icon_PingPong;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* Img_TypeIcon;
};
