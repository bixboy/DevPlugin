#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PatrolEditorWidget.generated.h"

class UUnitPatrolComponent;
class UScrollBox;
class UPatrolRouteRowWidget;

UCLASS()
class JUPITERPLUGIN_API UPatrolEditorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable)
	void SetupWithComponent(UUnitPatrolComponent* InComponent);

	UFUNCTION(BlueprintCallable)
	void SetupGlobal();

	UFUNCTION(BlueprintCallable)
	void Refresh();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UScrollBox* RouteList;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPatrolRouteRowWidget> RouteRowClass;

	UPROPERTY()
	UUnitPatrolComponent* PatrolComponent;

	bool bIsGlobalMode = false;
};
