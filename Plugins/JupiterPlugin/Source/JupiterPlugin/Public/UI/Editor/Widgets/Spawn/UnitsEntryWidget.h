#pragma once
#include "Data/Placement/PlacementUnitData.h"
#include "Player/JupiterPlayerSystem/CameraPlacementSystem.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UnitsEntryWidget.generated.h"

class UCustomButtonWidget;


UCLASS()
class JUPITERPLUGIN_API UUnitsEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	
	void SetPlacementSystem(UCameraPlacementSystem* InPlacementSystem);
	
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void InitEntry(UPlacementUnitData* Data);

	FText GetUnitDisplayName() const { return CachedUnitName; }
	
	const TArray<FName>& GetUnitTags() const { return UnitTags; }
    
	bool MatchesSearch(const FString& SearchLower) const;
	
	bool HasTag(FName Tag) const;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCustomButtonWidget> UnitButton;

protected:

	UFUNCTION()
	void OnUnitSelected(UCustomButtonWidget* Button, int Index);

	UPROPERTY(BlueprintReadOnly, Category = "Placement")
	TObjectPtr<UPlacementUnitData> PlacementData;

	UPROPERTY()
	TWeakObjectPtr<UCameraPlacementSystem> PlacementSystem;

	UPROPERTY()
	FText CachedUnitName;

	UPROPERTY()
	TArray<FName> UnitTags;
};