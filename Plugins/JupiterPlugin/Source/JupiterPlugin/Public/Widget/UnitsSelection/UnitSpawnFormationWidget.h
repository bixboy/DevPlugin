#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/UnitSpawnComponent.h"
#include "UnitSpawnFormationWidget.generated.h"

class UComboBoxString;
class UEditableTextBox;


UCLASS()
class JUPITERPLUGIN_API UUnitSpawnFormationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	virtual void NativeOnInitialized() override;

	virtual void NativeDestruct() override;

	UFUNCTION()
	void SetupWithComponent(UUnitSpawnComponent* InSpawnComponent);

protected:
	
	UFUNCTION()
	void OnFormationChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void InitializeFormationOptions();

	UFUNCTION()
	void UpdateSelection(ESpawnFormation NewFormation);
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UComboBoxString* FormationDropdown;

	UPROPERTY()
	UUnitSpawnComponent* SpawnComponent;

	TMap<FString, ESpawnFormation> OptionToFormation;
	
	TMap<ESpawnFormation, FString> FormationToOption;
	
	bool bUpdatingSelection = false;
};
