#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JupiterEditorPanel.generated.h"

class UPanelWidget;
class UWidgetSwitcher;
class UJupiterPageBase;
class UCameraPlacementSystem;
class UUnitPatrolComponent;
class UUnitSelectionComponent;
class UCustomButtonWidget;


UCLASS()
class JUPITERPLUGIN_API UJupiterEditorPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void SwitchToPage(int32 PageIndex);
    
    UWidgetSwitcher* GetContentSwitcher() const { return ContentSwitcher; }

protected:
	void FindComponents();

	void SetupSidebar();

	UFUNCTION()
	void OnSidebarButtonClicked(UCustomButtonWidget* Button, int32 Index);

	// --- UI Bindings ---

	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* ContentSwitcher;

	UPROPERTY(meta = (BindWidget))
	UPanelWidget* SidebarContainer;

	UPROPERTY(meta = (BindWidget))
	UPanelWidget* FooterContainer;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UCustomButtonWidget> SidebarButtonClass;

	// --- Component References ---

	UPROPERTY()
	UCameraPlacementSystem* PlacementSystem;

	UPROPERTY()
	UUnitPatrolComponent* PatrolComponent;

	UPROPERTY()
	UUnitSelectionComponent* SelectionComponent;
	
	UPROPERTY()
	TMap<int32, UCustomButtonWidget*> PageButtons;
};
