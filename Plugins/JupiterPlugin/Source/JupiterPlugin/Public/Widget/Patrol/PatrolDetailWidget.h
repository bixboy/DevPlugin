#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PatrolDetailWidget.generated.h"

class UUnitPatrolComponent;
class UCustomButtonWidget;
class UEditableTextBox;
class UTextBlock;
class UImage;

/**
 * Detailed view for a selected patrol route.
 * Handles actions like Focus, Select Units, Delete, and Rename.
 */
UCLASS(Abstract)
class JUPITERPLUGIN_API UPatrolDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bind this widget to a specific patrol and component */
	UFUNCTION(BlueprintCallable, Category = "Patrol Detail")
	void BindToPatrol(const FGuid& InPatrolID, UUnitPatrolComponent* Comp);

	/** Clear the current binding */
	UFUNCTION(BlueprintCallable, Category = "Patrol Detail")
	void ClearBinding();

	// --- Visual Components ---

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* Input_Name;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_Info;

	UPROPERTY(meta = (BindWidget))
	UCustomButtonWidget* Btn_Focus;

	UPROPERTY(meta = (BindWidget))
	UCustomButtonWidget* Btn_SelectUnits;
    
    UPROPERTY(meta = (BindWidget))
	UCustomButtonWidget* Btn_Loop;
    
    UPROPERTY(meta = (BindWidget))
	UCustomButtonWidget* Btn_Reverse;

	UPROPERTY(meta = (BindWidget))
	UCustomButtonWidget* Btn_Delete;
    
protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnFocusClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnSelectUnitsClicked(UCustomButtonWidget* Button, int Index);

    UFUNCTION()
	void OnLoopClicked(UCustomButtonWidget* Button, int Index);

    UFUNCTION()
	void OnReverseClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnDeleteClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnNameCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	/** Refreshes UI from the bound patrol data */
	void RefreshUI();

	TWeakObjectPtr<UUnitPatrolComponent> WeakPatrolComponent;
	FGuid BoundPatrolID;
    
    // Helpers copied/moved from EntryWidget
    void FocusCamera();
    void SelectAssignedUnits();
    void DeletePatrol();
};
