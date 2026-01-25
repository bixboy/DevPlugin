#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Patrol/UnitPatrolComponent.h"
#include "Components/Unit/UnitSpawnComponent.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "JupiterPageBase.generated.h"


UCLASS(Abstract)
class JUPITERPLUGIN_API UJupiterPageBase : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void InitPage(UUnitSpawnComponent* SpawnComp, UUnitPatrolComponent* PatrolComp, UUnitSelectionComponent* SelComp);
	
	virtual void OnPageOpened();
	
	virtual void OnPageClosed();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Page Settings")
	FText PageTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Page Settings")
	UTexture2D* PageIcon;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Dependencies")
	TWeakObjectPtr<UUnitSpawnComponent> SpawnComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Dependencies")
	TWeakObjectPtr<UUnitPatrolComponent> PatrolComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Dependencies")
	TWeakObjectPtr<UUnitSelectionComponent> SelectionComponent;


};
