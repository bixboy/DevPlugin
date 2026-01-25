#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Editor/JupiterUITypes.h"
#include "JupiterEditorPanel.generated.h"

class UPanelWidget;
class UWidgetSwitcher;
class UJupiterPageBase;
class UUnitSpawnComponent;
class UUnitPatrolComponent;
class UUnitSelectionComponent;
class UCustomButtonWidget;

/**
 * Root Controller for the RTS Editor UI.
 * Manages the Sidebar and Content Switcher.
 * Acts as the Single Source of Truth for accessing Components.
 */
UCLASS()
class JUPITERPLUGIN_API UJupiterEditorPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void SwitchToPage(int32 PageIndex);

protected:
	/** Finds components on the generic PlayerPawn/PlayerController */
	void FindComponents();

	/** Binds sidebar buttons */
	void SetupSidebar();

	UFUNCTION()
	void OnSidebarButtonClicked(UCustomButtonWidget* Button, int32 Index);

protected:
	// --- UI Bindings ---

	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* ContentSwitcher;

	UPROPERTY(meta = (BindWidget))
	UPanelWidget* SidebarContainer;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UCustomButtonWidget> SidebarButtonClass;

	// --- Component References ---

	UPROPERTY()
	UUnitSpawnComponent* SpawnComponent;

	UPROPERTY()
	UUnitPatrolComponent* PatrolComponent;

	UPROPERTY()
	UUnitSelectionComponent* SelectionComponent;
	
	// --- Internal State ---
	
	TMap<int32, UCustomButtonWidget*> PageButtons;
};
