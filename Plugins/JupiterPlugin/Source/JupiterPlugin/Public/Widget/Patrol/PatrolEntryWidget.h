#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/UnitPatrolComponent.h"
#include "PatrolEntryWidget.generated.h"


class UTextBlock;
class UCustomButtonWidget;
class UImage;
class UUnitPatrolComponent;

/**
 * Base widget class for a single patrol entry.
 * Exposes patrol details and functionality to Blueprint.
 */
UCLASS(Abstract)
class JUPITERPLUGIN_API UPatrolEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Internal ID to track which patrol this widget represents */
	UPROPERTY(BlueprintReadOnly, Category = "Patrol Entry")
	FGuid PatrolID;

	/** Number of units assigned to this patrol */
	UPROPERTY(BlueprintReadOnly, Category = "Patrol Entry")
	int32 UnitCount;

	/** Number of points in the patrol path */
	UPROPERTY(BlueprintReadOnly, Category = "Patrol Entry")
	int32 PointCount;

	/** Name of the patrol route */
	UPROPERTY(BlueprintReadOnly, Category = "Patrol Entry")
	FName RouteName;

	/** Color of the patrol route */
	UPROPERTY(BlueprintReadOnly, Category = "Patrol Entry")
	FLinearColor RouteColor;

	/** Human-readable description (e.g. "Loop", "Once") */
	UPROPERTY(BlueprintReadOnly, Category = "Patrol Entry")
	FText Description;

	// --- Visual Components ---

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_RouteName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_UnitCount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_PointCount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_Description;

	UPROPERTY(meta = (BindWidget))
	UImage* Image_RouteColor;

	UPROPERTY(meta = (BindWidget))
	UCustomButtonWidget* Btn_Focus;

	UPROPERTY(meta = (BindWidget))
	UCustomButtonWidget* Btn_SelectUnits;

	UPROPERTY(meta = (BindWidget))
	UCustomButtonWidget* Btn_Delete;

	// --- Logic ---

	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable, Category = "Patrol Entry")
	void Setup(const FPatrolRoute& Route, UUnitPatrolComponent* PatrolComp);


	UFUNCTION(BlueprintCallable, Category = "Patrol Entry")
	void FocusCamera();

	UFUNCTION(BlueprintCallable, Category = "Patrol Entry")
	void SelectAssignedUnits();

	UFUNCTION(BlueprintCallable, Category = "Patrol Entry")
	void DeletePatrol();

protected:
	UFUNCTION()
	void OnFocusClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnSelectUnitsClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnDeleteClicked(UCustomButtonWidget* Button, int Index);

	TWeakObjectPtr<UUnitPatrolComponent> WeakPatrolComponent;

	FVector FocusLocation;
};
