#pragma once

#include "CoreMinimal.h"
#include "SoldierRts.h"
#include "CommanderHive.generated.h"

UCLASS()
class JUPITERPLUGIN_API ACommanderHive : public ASoldierRts
{
	GENERATED_BODY()

public:
	ACommanderHive();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

// Commander Hive	
#pragma region Commander Hive
	
public:
	UFUNCTION()
	void OnAreaCommandBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,const FHitResult& SweepResult);
	UFUNCTION()
	void OnAreaCommandEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
protected:
	/*- Function -*/
	UFUNCTION()
	void UpdateMinions();
	UFUNCTION()
	virtual void TakeDamage_Implementation(AActor* DamageOwner) override;
	
	/*- Variables -*/
	UPROPERTY(EditAnywhere)
	TArray<AActor*> Minions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = true))
	TObjectPtr<USphereComponent> AreaCommand;

	UPROPERTY()
	AActor* CurrentTarget;

#pragma endregion	

};
