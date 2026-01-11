#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Unit/UnitSpawnComponent.h"
#include "UnitSpawnFormationWidget.generated.h"

class UComboBoxString;


UCLASS()
class JUPITERPLUGIN_API UUnitSpawnFormationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="RTS|UI")
	void SetupWithComponent(UUnitSpawnComponent* InSpawnComponent);

protected:
	UFUNCTION()
	void OnFormationChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	void InitializeFormationOptions();

	UFUNCTION()
	void UpdateSelectionFromComponent(ESpawnFormation NewFormation);
	void UpdateSelectionFromComponent();
    
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> FormationDropdown;

	UPROPERTY()
	TWeakObjectPtr<UUnitSpawnComponent> SpawnComponent;

	TMap<FString, ESpawnFormation> OptionToFormation;
	TMap<ESpawnFormation, FString> FormationToOption;
    
	bool bUpdatingSelection = false;
};