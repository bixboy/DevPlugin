#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PropsEntryWidget.generated.h"

class UPlacementItemData;
class UCustomButtonWidget;
class UCameraPlacementSystem;


/**
 * Single entry in the props catalogue (similar to UnitsEntryWidget but for props).
 */
UCLASS()
class JUPITERPLUGIN_API UPropsEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void InitEntry(UPlacementItemData* Data);
	void SetPlacementSystem(UCameraPlacementSystem* InPlacementSystem);

	bool MatchesSearch(const FString& SearchLower) const;
	bool HasTag(FName Tag) const;
	const TArray<FName>& GetItemTags() const { return ItemTags; }

	// --- Accessors ---
	FText GetDisplayName() const { return CachedDisplayName; }

	// --- Components ---
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCustomButtonWidget> ItemButton;

protected:
	UFUNCTION()
	void OnItemSelected(UCustomButtonWidget* Button, int Index);

private:
	UPROPERTY()
	TObjectPtr<UPlacementItemData> PlacementData;

	UPROPERTY()
	TWeakObjectPtr<UCameraPlacementSystem> PlacementSystem;

	UPROPERTY()
	FText CachedDisplayName;

	UPROPERTY()
	TArray<FName> ItemTags;
};
