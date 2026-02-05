#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Patrol/UnitPatrolComponent.h"
#include "Player/JupiterPlayerSystem/CameraPlacementSystem.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "JupiterPageBase.generated.h"


UCLASS(Abstract)
class JUPITERPLUGIN_API UJupiterPageBase : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void InitPage(UCameraPlacementSystem* PlacementSys, UUnitPatrolComponent* PatrolComp, UUnitSelectionComponent* SelComp);
	
	virtual void OnPageOpened();
	
	virtual void OnPageClosed();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Page Settings")
	FText PageTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Page Config")
    UTexture2D* PageIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Page Config")
    bool bIsFooterPage = false;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Dependencies")
	TWeakObjectPtr<UCameraPlacementSystem> PlacementSystem;

	UPROPERTY(BlueprintReadOnly, Category = "Dependencies")
	TWeakObjectPtr<UUnitPatrolComponent> PatrolComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Dependencies")
	TWeakObjectPtr<UUnitSelectionComponent> SelectionComponent;

};
