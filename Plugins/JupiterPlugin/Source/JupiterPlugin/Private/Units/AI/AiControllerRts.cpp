#include "JupiterPlugin/Public/Units/AI/AiControllerRts.h"
#include "NavigationSystem.h"
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

    if (!HasAuthority() || !bAttackTarget || !Soldier || !CurrentCommand.Target) return;

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
    CurrentCommand = Cmd;
    bAttackTarget = bAttack;
    bMoveComplete = false;
    bPatrolling = false;

    if (bAttackTarget)
    {
        MoveToActor(Cmd.Target, GetAcceptanceRadius());
    }
    else
    {
        MoveToLocation(Cmd.Location);
    }
    OnNewDestination.Broadcast(Cmd);
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
    if (!Soldier) return 0.f;
    return Soldier->GetHaveWeapon()
        ? RangedStopDistance
        : Soldier->GetAttackRange() * MeleeApproachFactor;
}

bool AAiControllerRts::ShouldApproach() const
{
    return bMoveComplete && GetDistanceToTarget() > Soldier->GetAttackRange();
}

#pragma endregion


// ---- Attack ---- //
#pragma region Attack Commands

void AAiControllerRts::PerformAttack()
{
    bCanAttack = false;
    OnStartAttack.Broadcast(CurrentCommand.Target);
    GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &AAiControllerRts::ResetAttack, Soldier->GetAttackCooldown(), false);
}

void AAiControllerRts::ResetAttack()
{
    bCanAttack = true;
}

void AAiControllerRts::StopAttack()
{
    GetWorld()->GetTimerManager().ClearTimer(AttackTimer);
    bCanAttack = true;
}


// Utilities ------
float AAiControllerRts::GetDistanceToTarget() const
{
    return FVector::Dist(Soldier->GetActorLocation(), CurrentCommand.Target->GetActorLocation());
}

bool AAiControllerRts::ShouldAttack() const
{
    return bCanAttack && bAttackTarget && GetDistanceToTarget() <= Soldier->GetAttackRange();
}

#pragma endregion


// ---- Patrol ---- //
#pragma region Patrol Commands

void AAiControllerRts::CommandPatrol(const FCommandData Cmd)
{
    CurrentCommand = Cmd;
    bPatrolling = true;
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
