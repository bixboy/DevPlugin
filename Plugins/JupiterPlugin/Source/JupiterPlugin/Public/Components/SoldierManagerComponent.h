#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoldierManagerComponent.generated.h"

class ASoldierRts;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JUPITERPLUGIN_API USoldierManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USoldierManagerComponent();

	UFUNCTION()
	void RegisterSoldier(ASoldierRts* Soldier);

	UFUNCTION()
	void UnregisterSoldier(ASoldierRts* Soldier);


protected:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY()
	const TArray<ASoldierRts*>& GetAllSoldiers() const
	{
		return Soldiers;
	}

	UFUNCTION(BlueprintCallable, Category="Settinsg")
	void PrintSoldierCount() const;
	
	UFUNCTION(Server, Reliable)
	void Server_RegisterSoldier(ASoldierRts* Soldier);

	UFUNCTION()
	void UpdateSoldierDetections();

	UPROPERTY()
	TArray<ASoldierRts*> Soldiers;

	UPROPERTY(EditAnywhere, Category="Settinsg")
	float DetectionInterval = 0.25f;

	UPROPERTY()
	float ElapsedTime = 0.f;
};
