#include "Systems/RtsOrderSystem.h"

#include "Interfaces/Selectable.h"
#include "Units/SoldierRts.h"
#include "Components/CommandComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

FCommandData FRTSOrderRequest::ToCommandData(AController* RequestingController) const
{
    FCommandData Result = LegacyCommand;
    Result.RequestingController = RequestingController;
    Result.Type = ToCommandType(OrderType);
    Result.SourceLocation = TargetLocation;
    Result.Location = TargetLocation;
    Result.Rotation = LegacyCommand.Rotation;
    Result.Target = TargetActor;
    Result.Radius = Radius;

    if (!Result.Target && LegacyCommand.Target)
    {
        Result.Target = LegacyCommand.Target;
    }

    switch (OrderType)
    {
    case ERTSOrderType::Stop:
    case ERTSOrderType::Hold:
        if (RequestingController && RequestingController->GetPawn())
        {
            Result.Location = RequestingController->GetPawn()->GetActorLocation();
            Result.SourceLocation = Result.Location;
            Result.Rotation = RequestingController->GetPawn()->GetActorRotation();
        }
        Result.Target = nullptr;
        break;
    case ERTSOrderType::Harvest:
        if (!Result.Target)
        {
            Result.Target = TargetActor;
        }
        break;
    default:
        break;
    }

    return Result;
}

ERTSOrderType FRTSOrderRequest::FromCommandType(ECommandType CommandType)
{
    switch (CommandType)
    {
    case ECommandType::CommandMoveFast:
        return ERTSOrderType::MoveFast;
    case ECommandType::CommandMoveSlow:
        return ERTSOrderType::MoveSlow;
    case ECommandType::CommandAttack:
        return ERTSOrderType::Attack;
    case ECommandType::CommandPatrol:
        return ERTSOrderType::Patrol;
    default:
        return ERTSOrderType::Move;
    }
}

ECommandType FRTSOrderRequest::ToCommandType(ERTSOrderType OrderType)
{
    switch (OrderType)
    {
    case ERTSOrderType::MoveFast:
        return ECommandType::CommandMoveFast;
    case ERTSOrderType::MoveSlow:
        return ECommandType::CommandMoveSlow;
    case ERTSOrderType::Attack:
    case ERTSOrderType::Harvest:
        return ECommandType::CommandAttack;
    case ERTSOrderType::Patrol:
        return ECommandType::CommandPatrol;
    case ERTSOrderType::Stop:
    case ERTSOrderType::Hold:
        return ECommandType::CommandMove;
    default:
        return ECommandType::CommandMove;
    }
}

void URTSOrderSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void URTSOrderSystem::Deinitialize()
{
    Super::Deinitialize();
}

void URTSOrderSystem::IssueOrder(AController* IssuingController, const TArray<AActor*>& Units, const FRTSOrderRequest& OrderRequest)
{
    if (!IssuingController || Units.IsEmpty())
    {
        return;
    }

    FRTSOrderRequest FinalOrder = OrderRequest;
    FinalOrder.LegacyCommand = OrderRequest.ToCommandData(IssuingController);

    bool bHasLocalExecution = false;
    for (AActor* Unit : Units)
    {
        if (Unit)
        {
            bHasLocalExecution |= IsLocalExecution(IssuingController, Unit);
            DispatchOrderToUnit(IssuingController, Unit, FinalOrder);
        }
    }

    OnOrderIssued.Broadcast(IssuingController, Units, FinalOrder, FinalOrder.bQueue, bHasLocalExecution);
}

void URTSOrderSystem::IssueOrder(AController* IssuingController, const TArray<AActor*>& Units, const FCommandData& CommandData, bool bQueueCommand)
{
    FRTSOrderRequest Request(CommandData);
    Request.bQueue = bQueueCommand;
    IssueOrder(IssuingController, Units, Request);
}

void URTSOrderSystem::DispatchOrderToUnit(AController* IssuingController, AActor* Unit, const FRTSOrderRequest& OrderRequest) const
{
    if (!Unit)
    {
        return;
    }

    const FCommandData CommandData = OrderRequest.ToCommandData(IssuingController);

    if (ASoldierRts* Soldier = Cast<ASoldierRts>(Unit))
    {
        Soldier->IssueOrder(OrderRequest.OrderType, OrderRequest.TargetLocation, OrderRequest.TargetActor, OrderRequest.Radius, OrderRequest.OrderTags, OrderRequest.bQueue, CommandData);
        return;
    }

    if (Unit->Implements<USelectable>())
    {
        ISelectable::Execute_CommandMove(Unit, CommandData);
    }
}

bool URTSOrderSystem::IsLocalExecution(AController* IssuingController, AActor* Unit)
{
    if (!Unit)
    {
        return false;
    }

    if (IssuingController && IssuingController->IsLocalController())
    {
        return true;
    }

    if (const APawn* Pawn = Cast<APawn>(Unit))
    {
        if (const AController* UnitController = Pawn->GetController())
        {
            return UnitController->IsLocalController();
        }
    }

    return false;
}

