// Copyright Epic Games, Inc. All Rights Reserved.

#include "JupiterPlugin/Public/RtsGameGameMode.h"
#include "JupiterPlugin/Public/RtsGamePlayerController.h"
#include "Player/PlayerControllerRts.h"
#include "Components/SlectionComponent.h"
#include "Systems/RtsOrderSystem.h"

ARtsGameGameMode::ARtsGameGameMode()
{
        PlayerControllerClass = ARtsGamePlayerController::StaticClass();
}

void ARtsGameGameMode::IssueExampleMoveOrder(APlayerControllerRts* Controller, const FVector& TargetLocation)
{
        if (!Controller || !Controller->SelectionComponent)
        {
                return;
        }

        FRTSOrderRequest Request;
        Request.OrderType = ERTSOrderType::Move;
        Request.TargetLocation = TargetLocation;
        Controller->SelectionComponent->IssueOrderToSelection(Request);
}
