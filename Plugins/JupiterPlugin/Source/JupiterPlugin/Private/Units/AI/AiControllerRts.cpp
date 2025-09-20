#include "JupiterPlugin/Public/Units/AI/AiControllerRts.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/AiData.h"
#include "Interfaces/Selectable.h"
#include "Units/SoldierRts.h"

// ---- Setup ---- //
#pragma region Setup Fonction

AAiControllerRts::AAiControllerRts()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AAiControllerRts::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
        
    Soldier = Cast<ASoldierRts>(InPawn);
    if (Soldier)
        Soldier->SetAIController(this);
}

void AAiControllerRts::SetupVariables()
{
    if (Soldier)
        CombatBehavior = Soldier->GetCombatBehavior();
}

#pragma endregion

void AAiControllerRts::Tick(float DeltaSeconds)
{
        Super::Tick(DeltaSeconds);

        if (!HasAuthority() || !Soldier)
                return;

        if (!bAttackTarget)
                return;

        if (!HasValidAttackCommand())
        {
                HandleInvalidAttackTarget();
                return;
        }

        if (ShouldAttack())
        {
                if (!bMoveComplete)
                {
                        StopMovement();
                        bMoveComplete = true;
                }

                PerformAttack();
        }
        else if (ShouldApproach())
        {
                bMoveComplete = false;
                MoveToActor(CurrentCommand.Target, GetAcceptanceRadius());
        }
}


// ---- Movement ---- //
#pragma region Movement Commands

void AAiControllerRts::CommandMove(const FCommandData Cmd, bool bAttack)
{
        bool bShouldAttack = bAttack;

        if (bShouldAttack && Soldier && HasAuthority() && Soldier->GetCombatBehavior() == ECombatBehavior::Passive)
        {
                ISelectable::Execute_SetBehavior(Soldier, ECombatBehavior::Neutral);
        }

        if (bShouldAttack && !ValidateAttackCommand(Cmd))
        {
                bShouldAttack = false;
        }

        if (!bShouldAttack && bAttackTarget)
        {
                StopAttack();
        }

        CurrentCommand = Cmd;
        bPatrolling = false;
        bMoveComplete = false;
        bAttackTarget = bShouldAttack;

        if (bAttackTarget && CurrentCommand.Target)
        {
                CurrentCommand.Location = CurrentCommand.Target->GetActorLocation();
                MoveToActor(CurrentCommand.Target, GetAcceptanceRadius());
        }
        else
        {
                MoveToLocation(CurrentCommand.Location);
        }

        OnNewDestination.Broadcast(CurrentCommand);
}

void AAiControllerRts::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);
    
    bMoveComplete = true;
    OnReachedDestination.Broadcast(CurrentCommand);
    
    if (bPatrolling)
    {
        StartPatrol();
    }
}


// Utilities ------
float AAiControllerRts::GetAcceptanceRadius() const
{
        if (!Soldier)
        {
                return 0.f;
        }

        if (Soldier->GetHaveWeapon())
        {
                const float SoldierStopDistance = Soldier->GetRangedStopDistance();
                return SoldierStopDistance > 0.f ? SoldierStopDistance : RangedStopDistance;
        }

        float ComputedStopDistance = 0.f;

        if (const USkeletalMeshComponent* MeshComp = Soldier->GetMesh())
        {
                const FBoxSphereBounds Bounds = MeshComp->Bounds;
                const float HorizontalExtent = FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y);
                const float StopFactor = FMath::Max(Soldier->GetMeleeStopDistanceFactor(), 0.f);
                ComputedStopDistance = HorizontalExtent * 2.f * StopFactor;
        }

        if (ComputedStopDistance <= KINDA_SMALL_NUMBER)
        {
                ComputedStopDistance = Soldier->GetAttackRange() * MeleeApproachFactor;
        }
        else if (Soldier->GetAttackRange() > 0.f)
        {
                ComputedStopDistance = FMath::Min(ComputedStopDistance, Soldier->GetAttackRange());
        }

        return ComputedStopDistance;
}

bool AAiControllerRts::ShouldApproach() const
{
        if (!HasValidAttackCommand() || !Soldier)
        {
                return false;
        }

        return bMoveComplete && GetDistanceToTarget() > Soldier->GetAttackRange();
}

#pragma endregion


// ---- Attack ---- //
#pragma region Attack Commands

void AAiControllerRts::PerformAttack()
{
        if (!HasValidAttackCommand())
        {
                return;
        }

        bCanAttack = false;
        OnStartAttack.Broadcast(CurrentCommand.Target);

        if (UWorld* World = GetWorld())
        {
                World->GetTimerManager().SetTimer(AttackTimer, this, &AAiControllerRts::ResetAttack, Soldier->GetAttackCooldown(), false);
        }
}

void AAiControllerRts::ResetAttack()
{
        bCanAttack = true;
}

void AAiControllerRts::StopAttack()
{
        if (UWorld* World = GetWorld())
        {
                World->GetTimerManager().ClearTimer(AttackTimer);
        }

        bCanAttack = true;
        bAttackTarget = false;
        bMoveComplete = true;
        CurrentCommand.Target = nullptr;

        StopMovement();
}


// Utilities ------
float AAiControllerRts::GetDistanceToTarget() const
{
        if (!Soldier || !HasValidAttackCommand())
        {
                return 0.f;
        }

        return FVector::Dist(Soldier->GetActorLocation(), CurrentCommand.Target->GetActorLocation());
}

bool AAiControllerRts::ShouldAttack() const
{
        if (!HasValidAttackCommand() || !Soldier)
        {
                return false;
        }

        return bCanAttack && GetDistanceToTarget() <= Soldier->GetAttackRange();
}

#pragma endregion


bool AAiControllerRts::HasValidAttackCommand() const
{
        return bAttackTarget && ValidateAttackCommand(CurrentCommand);
}

bool AAiControllerRts::ValidateAttackCommand(const FCommandData& Cmd) const
{
        if (!Soldier)
                return false;

        return Cmd.Target && IsValid(Cmd.Target) && Cmd.Target != Soldier;
}

void AAiControllerRts::HandleInvalidAttackTarget()
{
        StopAttack();
}

 
// ---- Patrol ---- //
#pragma region Patrol Commands

void AAiControllerRts::CommandPatrol(const FCommandData Cmd)
{
        StopAttack();
        CurrentCommand = Cmd;
        bPatrolling = true;
        bMoveComplete = false;
        
        StartPatrol();
}

void AAiControllerRts::StartPatrol()
{
    FVector Dest;
    if (UNavigationSystemV1::K2_GetRandomLocationInNavigableRadius(GetWorld(), CurrentCommand.SourceLocation,
            Dest, CurrentCommand.Radius))
    {
        MoveToLocation(Dest, 20.f);
    }
}

#pragma endregion
