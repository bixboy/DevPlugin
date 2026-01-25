#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PatrolDetailWidget.generated.h"

class UUnitPatrolComponent;
class UCustomButtonWidget;
class UEditableTextBox;
class UTextBlock;
class UImage;
class UWrapBox;


UCLASS(Abstract)
class JUPITERPLUGIN_API UPatrolDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Patrol Detail")
	void SetupDetailWidget(int32 Index, UUnitPatrolComponent* Comp);

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
	UWrapBox* ActionButtonsContainer;

	UPROPERTY(meta = (BindWidget))
	UCustomButtonWidget* Btn_DeletePatrol;
    
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
	void OnDeletePatrolClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnNameCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	/** Refreshes UI from the bound patrol data */
	void RefreshUI();

	TWeakObjectPtr<UUnitPatrolComponent> WeakPatrolComponent;
	FGuid BoundPatrolID;
	int32 CurrentPatrolIndex = -1;
    
    // Helpers copied/moved from EntryWidget
    void FocusCamera();
    void SelectAssignedUnits();
    void DeletePatrol();
};
