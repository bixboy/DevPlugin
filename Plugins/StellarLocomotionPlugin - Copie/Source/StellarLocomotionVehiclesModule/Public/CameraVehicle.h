#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraVehicle.generated.h"

class UCameraComponent;

UCLASS()
class STELLARLOCOMOTIONVEHICLESMODULE_API ACameraVehicle : public APawn
{
	GENERATED_BODY()

public:
	ACameraVehicle();
	
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable)
	UCameraComponent* GetCameraComponent();
	
	UFUNCTION()
	bool GetIsUsed() const;
	
	UFUNCTION()
	void SetIsUsed(bool bIsUsed);
	
	UFUNCTION()
	void SetController(APlayerController* NewOwner);
	
	UFUNCTION()
	APlayerController* GetCameraController() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CameraComp;

	UPROPERTY()
	bool IsUsed = false;
	
	UPROPERTY()
	APlayerController* OwningPlayer;
};
