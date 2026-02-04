#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UnitSpawnCountWidget.generated.h"

class UEditableTextBox;
class UCustomButtonWidget;
class UCameraPlacementSystem;


UCLASS()
class JUPITERPLUGIN_API UUnitSpawnCountWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	virtual void NativeOnInitialized() override;

	virtual void NativeDestruct() override;

	UFUNCTION()
	void SetupWithSystem(UCameraPlacementSystem* InPlacementSystem);

protected:
	
	UFUNCTION()
	void OnSpawnCountCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleSpawnCountChanged(int32 NewCount);

	UFUNCTION()
	void RefreshDisplay() const;

	UFUNCTION()
	void OnSliderValueChanged(float Value);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEditableTextBox* SpawnCountTextBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	class UCustomSliderWidget* SpawnCountSlider;

	UPROPERTY()
	TWeakObjectPtr<UCameraPlacementSystem> PlacementSystem;

	int32 CachedSpawnCount = 1;
	int32 MaxSpawnCount = 50;
	
};
