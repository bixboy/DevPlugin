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
	
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

// Commander Hive
	
	UFUNCTION()
	void UpdateMinions();
	
	UFUNCTION()
	virtual void TakeDamage_Implementation(AActor* DamageOwner) override;
	
};
