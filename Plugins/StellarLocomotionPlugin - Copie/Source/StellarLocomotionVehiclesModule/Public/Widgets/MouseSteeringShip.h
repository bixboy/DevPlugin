#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MouseSteeringShip.generated.h"

class UImage;


UCLASS()
class STELLARLOCOMOTIONVEHICLESMODULE_API UMouseSteeringShip : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void SetupWidget(float NewNeutralThreshold, float NewMaxThreshold);

	UFUNCTION()
	void UpdateMouseVisuals();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(BindWidget))
	UImage* NeutralZoneImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(BindWidget))
	UImage* MaxZoneImage;
	
	UPROPERTY(meta = (BindWidget))
	UImage* MouseLineImage;

	UPROPERTY()
	float NeutralThreshold = 0.2f;

	UPROPERTY()
	float MaxThreshold = 0.8f;
};
