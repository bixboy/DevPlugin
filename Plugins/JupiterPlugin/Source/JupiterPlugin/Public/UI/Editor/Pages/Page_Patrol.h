#pragma once
#include "CoreMinimal.h"
#include "UI/Editor/JupiterPageBase.h"
#include "Page_Patrol.generated.h"

class UUnitPatrolComponent;
class UPatrolEntryWidget;
class UPatrolDetailWidget;
class UPanelWidget;


UCLASS()
class JUPITERPLUGIN_API UPage_Patrol : public UJupiterPageBase
{
	GENERATED_BODY()

public:
	virtual void InitPage(UUnitSpawnComponent* SpawnComp, UUnitPatrolComponent* PatrolComp, UUnitSelectionComponent* SelComp) override;
	virtual void OnPageOpened() override;
	virtual void OnPageClosed() override;

protected:

	UFUNCTION(BlueprintCallable, Category = "Patrol List")
	void RefreshList();

	UFUNCTION()
	void HandleRoutesChanged();

	UFUNCTION()
	void OnEntrySelected(int32 LoopIndex, UPatrolEntryWidget* Entry);

protected:
	// --- Config ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol List")
	float MaxDisplayDistance = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol List")
	TSubclassOf<UPatrolEntryWidget> EntryWidgetClass;

	// --- UI Elements ---

	UPROPERTY(meta = (BindWidget))
	UPanelWidget* EntryContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	UPatrolDetailWidget* PatrolDetail;

	// --- State ---
	FDelegateHandle OnRoutesChangedHandle;
	FGuid SelectedPatrolID;
};
