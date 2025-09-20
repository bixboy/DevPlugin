#include "JupiterPlugin/Public/Units/AI/AiControllerRts.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Data/AiData.h"
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
    {
        Soldier->SetAIController(this);
    }
}

void AAiControllerRts::SetupVariables()
{
    if (Soldier)
    {
        CombatBehavior = Soldier->GetCombatBehavior();
    }
}

#pragma endregion

void AAiControllerRts::Tick(float DeltaSeconds)
{
        Super::Tick(DeltaSeconds);

        if (!HasAuthority() || !Soldier)
        {
                return;
        }

        if (!bAttackTarget)
        {
                return;
        }

        if (!HasValidAttackCommand())
        {
                HandleInvalidAttackTarget();
                return;
        }

        if (IsOutsideLeashRange())
        {
                StopAttack();
                return;
        }

        if (ShouldRetreat())
        {
                MaintainDistance();
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

        const float PreferredDistance = Soldier->GetPreferredAttackDistance();
        return FMath::Max(PreferredDistance, DistanceTolerance);
}

bool AAiControllerRts::ShouldApproach() const
{
        if (!HasValidAttackCommand() || !Soldier)
        {
                return false;
        }

        return bMoveComplete && GetDistanceToTarget() > Soldier->GetAttackRange() + DistanceTolerance;
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

        const float Distance = GetDistanceToTarget();
        const float MinDistance = Soldier->GetMinAttackDistance();
        const float MaxRange = Soldier->GetAttackRange();

        const bool bAboveMinimumRange = MinDistance <= 0.f || Distance >= MinDistance - DistanceTolerance;
        return bCanAttack && bAboveMinimumRange && Distance <= MaxRange + DistanceTolerance;
}

bool AAiControllerRts::ShouldRetreat() const
{
        if (!HasValidAttackCommand() || !Soldier)
        {
                return false;
        }

        const float MinDistance = Soldier->GetMinAttackDistance();
        if (MinDistance <= 0.f)
        {
                return false;
        }

        return GetDistanceToTarget() < MinDistance - DistanceTolerance;
}

void AAiControllerRts::MaintainDistance()
{
        if (!HasValidAttackCommand() || !Soldier)
        {
                return;
        }

        const FVector SoldierLocation = Soldier->GetActorLocation();
        const FVector TargetLocation = CurrentCommand.Target->GetActorLocation();
        FVector RetreatDirection = SoldierLocation - TargetLocation;
        if (!RetreatDirection.Normalize())
        {
                return;
        }

        const float DesiredDistance = Soldier->GetPreferredAttackDistance();
        const float MinimumDistance = Soldier->GetMinAttackDistance();
        const float RetreatDistance = FMath::Max(DesiredDistance, MinimumDistance + RetreatBuffer);
        const FVector Destination = TargetLocation + RetreatDirection * RetreatDistance;

        bMoveComplete = false;
        MoveToLocation(Destination, DistanceTolerance);
}

bool AAiControllerRts::IsOutsideLeashRange() const
{
        if (!HasValidAttackCommand() || !Soldier)
        {
                return false;
        }

        return GetDistanceToTarget() > Soldier->GetAttackChaseDistance() + DistanceTolerance;
}

#pragma endregion


bool AAiControllerRts::HasValidAttackCommand() const
{
        return bAttackTarget && ValidateAttackCommand(CurrentCommand);
}

bool AAiControllerRts::ValidateAttackCommand(const FCommandData& Cmd) const
{
        if (!Soldier)
        {
                return false;
        }

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
    if (UNavigationSystemV1::K2_GetRandomLocationInNavigableRadius(
        GetWorld(), CurrentCommand.SourceLocation, Dest, CurrentCommand.Radius))
    {
        MoveToLocation(Dest, 20.f);
    }
}

#pragma endregion
