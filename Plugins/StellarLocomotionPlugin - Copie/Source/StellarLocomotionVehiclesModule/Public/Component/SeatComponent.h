#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SeatComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STELLARLOCOMOTIONVEHICLESMODULE_API USeatComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	USeatComponent();

	UPROPERTY(EditAnywhere, Category="Seat")
	int32 SeatIndex = 0;

	UPROPERTY(EditAnywhere, Category="Seat")
	bool bIsDriverSeat = false;

protected:
	virtual void BeginPlay() override;
	
};
