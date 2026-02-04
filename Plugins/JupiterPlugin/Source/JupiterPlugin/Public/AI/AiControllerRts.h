#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "Data/AiData.h"
#include "Interfaces/PatrolAgentInterface.h"
#include "AiControllerRts.generated.h"

class ASoldierRts;
struct FCommandData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReachedDestination, const FCommandData, CommandData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNewDestination, const FCommandData, CommandData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartAttack, AActor*, Target);


UCLASS()
class JUPITERPLUGIN_API AAiControllerRts : public AAIController, public IPatrolAgentInterface
{
	GENERATED_BODY()

public:
	AAiControllerRts();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UFUNCTION(BlueprintCallable, Category="AI")
	void CommandMove(const FCommandData Cmd, bool bAttack = false);

	UFUNCTION(BlueprintCallable, Category="AI")
	void CommandPatrol(const FCommandData Cmd);

    UFUNCTION(BlueprintCallable, Category="AI")
    virtual void UpdatePatrolRoute(const TArray<FVector>& Points, bool bLoop, int32 StartIndex) override;

    UFUNCTION(BlueprintCallable, Category="AI")
    virtual void StopPatrol() override;

	UFUNCTION(BlueprintCallable, Category="AI")
	void ResetAttack();

	UFUNCTION(BlueprintCallable, Category="AI")
	void StopAttack();

	// Delegates
	UPROPERTY(BlueprintAssignable, Category="AI")
	FOnNewDestination OnNewDestination;

	UPROPERTY(BlueprintAssignable, Category="AI")
	FReachedDestination OnReachedDestination;

	UPROPERTY(BlueprintAssignable, Category="AI")
	FOnStartAttack OnStartAttack;

	UFUNCTION(BlueprintPure, Category="AI")
	FCommandData GetCurrentCommand() const { return CurrentCommand; }
	
	UFUNCTION(BlueprintPure, Category="AI")
	bool IsMoveComplete() const { return bMoveComplete; }

	// Attack
	UFUNCTION(BlueprintPure, Category="AI")
	bool HasAttackTarget() const { return bAttackTarget; }

	UFUNCTION(BlueprintPure, Category="AI")
	bool CanAttack() const { return bCanAttack; }

	UFUNCTION(BlueprintCallable, Category="AI")
	void SetAttackTarget(bool bAttack) { bAttackTarget = bAttack; }

	UFUNCTION(BlueprintCallable, Category="AI")
	virtual int32 GetCurrentPatrolWaypointIndex() const override { return CurrentPatrolWaypointIndex; }

	UFUNCTION(BlueprintCallable, Category="AI")
	ECombatBehavior GetCombatBehavior() const { return CombatBehavior; }

	UFUNCTION(BlueprintCallable, Category="AI")
	void SetupVariables();

protected:
	UPROPERTY(EditAnywhere, Category="AI")
	float MeleeApproachFactor = 0.3f;

	UPROPERTY(EditAnywhere, Category="AI")
	float RangedStopDistance = 200.f;

	UPROPERTY(EditAnywhere, Category="AI")
	float AttackCooldown = 1.f;

private:
        UPROPERTY() ASoldierRts* Soldier = nullptr;
        UPROPERTY() FCommandData CurrentCommand;

        UPROPERTY() bool bMoveComplete = true;
        UPROPERTY() bool bPatrolling = false;
        UPROPERTY() bool bWasPatrolling = false; // State to resume patrol after combat

        UPROPERTY() bool bAttackTarget = false;
        UPROPERTY() bool bCanAttack = true;
	
        UPROPERTY() FTimerHandle AttackTimer;
        UPROPERTY() ECombatBehavior CombatBehavior;

        // Functions
        UFUNCTION() float GetAcceptanceRadius() const;
        UFUNCTION() bool  ShouldApproach() const;

        UFUNCTION() float GetDistanceToTarget() const;
        UFUNCTION() bool  ShouldAttack() const;
        UFUNCTION() void  PerformAttack();

        bool HasValidAttackCommand() const;
        bool ValidateAttackCommand(const FCommandData& Cmd) const;
        void HandleInvalidAttackTarget();

        UFUNCTION() void StartPatrol();
        void AdvancePatrolWaypoint();

        UPROPERTY()
        TArray<FVector> CurrentPatrolPath;

        UPROPERTY()
        bool bPatrolLoopPattern = false;

        UPROPERTY()
        int32 CurrentPatrolWaypointIndex = 0;

        UPROPERTY()
        bool bPatrolForward = true;
};
