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

	UFUNCTION()
	void RegisterSoldier(ASoldierRts* Soldier);

	UFUNCTION()
	void UnregisterSoldier(ASoldierRts* Soldier);

	UFUNCTION()
	void UpdateSoldierDetections(int32 BucketIndex);

	UFUNCTION()
	const TArray<ASoldierRts*>& GetAllSoldiers() const { return Soldiers; }

protected:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(Server, Reliable)
	void Server_RegisterSoldier(ASoldierRts* Soldier);

	UFUNCTION()
	bool AddSoldierInternal(ASoldierRts* Soldier);

	UFUNCTION()
	bool RemoveSoldierInternal(ASoldierRts* Soldier);

	UFUNCTION(BlueprintCallable, Category="Settings")
	void PrintSoldierCount() const;

    UPROPERTY()
    TArray<ASoldierRts*> Soldiers;

    UPROPERTY(EditAnywhere, Category="Settings|Manager")
    float DetectionInterval = 0.25f;

    UPROPERTY(EditAnywhere, Category="Settings|Manager")
    int32 NumBuckets = 4;

    int32 CurrentBucketIndex = 0;
    float ElapsedTime = 0.f;
};
