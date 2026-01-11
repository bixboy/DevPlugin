#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoldierManagerComponent.generated.h"

class ASoldierRts;


UCLASS(ClassGroup=(JupiterPlugin), meta=(BlueprintSpawnableComponent))
class JUPITERPLUGIN_API USoldierManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USoldierManagerComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="RTS|Manager")
	void RegisterSoldier(ASoldierRts* Soldier);

	UFUNCTION(BlueprintCallable, Category="RTS|Manager")
	void UnregisterSoldier(ASoldierRts* Soldier);

	const TArray<ASoldierRts*>& GetAllSoldiers() const { return Soldiers; }

protected:
	void ProcessDetectionBucket(int32 BucketIndex);
    
	UFUNCTION(Server, Reliable)
	void Server_RegisterSoldier(ASoldierRts* Soldier);

private:
	UPROPERTY(Transient)
	TArray<ASoldierRts*> Soldiers;

	// Settings
	UPROPERTY(EditAnywhere, Category="Settings|Manager")
	float FullLoopDuration = 0.5f;

	UPROPERTY(EditAnywhere, Category="Settings|Manager")
	int32 TargetBuckets = 10;

	// State
	int32 CurrentBucketIndex = 0;
	float TimeSinceLastBucket = 0.f;
};