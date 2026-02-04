#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Placement/PlacementTypes.h"
#include "UnitSpawnAxisWidget.generated.h"

class UEditableTextBox;
class UCameraPlacementSystem;


UCLASS()
class JUPITERPLUGIN_API UUnitSpawnAxisWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	virtual void NativeOnInitialized() override;

	virtual void NativeDestruct() override;
	
	UFUNCTION()
	void SetupWithSystem(UCameraPlacementSystem* InPlacementSystem);

protected:
	
	UFUNCTION()
	void OnCustomFormationXCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnCustomFormationYCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void RefreshCustomFormationInputs(ESpawnFormation NewFormation);

    UFUNCTION()
    void HandleCustomFormationDimensionsChanged(FIntPoint NewDimensions);

    void ApplyCustomFormationToSpawnCount() const;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UEditableTextBox* FormationX;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEditableTextBox* FormationY;
	
	UPROPERTY()
	TWeakObjectPtr<UCameraPlacementSystem> PlacementSystem;

private:
	bool bIsUpdatingFromUI = false;
};
