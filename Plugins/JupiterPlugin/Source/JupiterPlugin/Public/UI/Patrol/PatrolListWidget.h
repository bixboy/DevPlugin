#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PatrolListWidget.generated.h"

class UUnitPatrolComponent;
class UPatrolEntryWidget;
class UPatrolDetailWidget;
class UPanelWidget;

/**
 * Base widget class for displaying a list of patrols.
 * Handles creating and updating entry widgets based on component data.
 */
UCLASS(Abstract)
class JUPITERPLUGIN_API UPatrolListWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Patrol List")
	void InitializePatrolList(UUnitPatrolComponent* Comp);

	UFUNCTION(BlueprintCallable, Category = "Patrol List")
	void RefreshList();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol List")
	float MaxDisplayDistance = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol List")
	TSubclassOf<UPatrolEntryWidget> EntryWidgetClass;

	UPROPERTY(meta = (BindWidget))
	UPanelWidget* EntryContainer;

	UPROPERTY(meta = (BindWidget))
	UPatrolDetailWidget* PatrolDetail;

	UPROPERTY(BlueprintReadOnly, Category = "Patrol List")
	TWeakObjectPtr<UUnitPatrolComponent> PatrolComponent;

	FDelegateHandle OnRoutesChangedHandle;
	FGuid SelectedPatrolID;

	UFUNCTION()
	void HandleRoutesChanged();

	UFUNCTION()
	void OnEntrySelected(FGuid PatrolID);
};
