#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "WeaponMaster.generated.h"

class ASoldierRts;
class AProjetiles;

UCLASS()
class JUPITERPLUGIN_API UWeaponMaster : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UWeaponMaster();
	virtual void BeginPlay() override;

        UFUNCTION()
        void AIShoot(AActor* Target);

        UFUNCTION()
        void SetAiOwner(ASoldierRts* NewOwner);
        UFUNCTION(Server, Reliable)
        void SetAiOwner_Server(ASoldierRts* NewOwner);

        UFUNCTION(BlueprintCallable, Category = "Weapon")
        float GetWeaponRange() const { return WeaponRange; }

protected:
	/*- Function -*/
	UFUNCTION(BlueprintCallable)
	FVector GetDirection();
	UFUNCTION(BlueprintCallable)
	TArray<FVector> GetShootTrace(FVector Direction);

	UFUNCTION(BlueprintCallable)
	void SpawnBullet();
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void SetHasAiming_Server(bool Value);
	
	UFUNCTION()
	FHitResult PerformSingleLineTrace(const FVector& Start, const FVector& End, ECollisionChannel TraceChannel) const;

	/*- Variables -*/
	UPROPERTY(EditAnywhere, Category = "Settings|Weapon Values")
	TObjectPtr<USkeletalMesh> WeaponMesh;
	UPROPERTY(EditAnywhere, Category = "Settings|Weapon Values")
	TSubclassOf<AProjetiles> BulletClass;
	
	UPROPERTY(EditAnywhere, Category = "Settings|Weapon Values")
	float WeaponRange = 1000.f;
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Settings|Weapon Values")
	bool HasAiming = false;

	UPROPERTY()
	TArray<FVector> StartEndPoints;
	UPROPERTY()
	APlayerController* PlayerController;

	UPROPERTY(BlueprintReadWrite)
	bool ControlledByAi = false;
	UPROPERTY(BlueprintReadOnly, Replicated, EditAnywhere)
	ASoldierRts* AiOwner;

	/*- Server Replication -*/
	UFUNCTION(Server, Reliable)
	void SpawnBullet_Server();
	UFUNCTION(Server, Reliable)
	void GetShootTrace_Server(const FVector Direction);

	UFUNCTION(Server, Reliable)
	void AiSpawnBullet_Server(AActor* Target);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

