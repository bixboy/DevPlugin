#include "CustomPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Component/ProximityPromptComponent.h"
#include "Component/ProximityPromptManagerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Vehicles/VehicleMaster.h"


// ----------- Setup Functions -----------
#pragma region Setup Functions

void ACustomPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (APawn* P = GetPawn())
    {
        PromptManager = P->FindComponentByClass<UProximityPromptManagerComponent>();
        Server_SetPromptManager(PromptManager);
        
        if (PromptManager)
        {
            PromptManager->OnPromptInteracted.AddDynamic(this, &ACustomPlayerController::OnPromptInteracted);
        }
    }
}

void ACustomPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ACustomPlayerController, bInVehicle);
    DOREPLIFETIME(ACustomPlayerController, CurrentVehicle);
    DOREPLIFETIME(ACustomPlayerController, CurrentCamera);
}

void ACustomPlayerController::Client_AddMappingContext_Implementation(const UInputMappingContext* MappingContext)
{
    if (auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Sub->AddMappingContext(MappingContext, 1);
    }
}

void ACustomPlayerController::Client_RemoveMappingContext_Implementation(const UInputMappingContext* MappingContext)
{
    if (auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Sub->RemoveMappingContext(MappingContext);
    }
}

#pragma endregion



// ----------- Vehicle Actions -----------
#pragma region Vehicle Actions

void ACustomPlayerController::EnterInVehicle(AVehicleMaster* Vehicle, USceneComponent* ChosenSeat)
{
    if (Vehicle)
    {
        Server_EnterInVehicle(Vehicle, ChosenSeat);
    }
}

void ACustomPlayerController::ExitVehicle()
{
    if (CurrentVehicle)
    {
        Server_ExitVehicle(CurrentVehicle);
    }
}

void ACustomPlayerController::SwitchViewMode()
{
    Server_SwitchViewMode();
}

void ACustomPlayerController::Server_EnterInVehicle_Implementation(AVehicleMaster* Vehicle, USceneComponent* ChosenSeat)
{
    if (!bInVehicle && Vehicle)
    {
        if (Execute_Interact(Vehicle, this, ChosenSeat))
        {
            bInVehicle = true;
            CurrentVehicle = Vehicle;

            SetPromptEnabled(false);
        }
    }
}

void ACustomPlayerController::Server_ExitVehicle_Implementation(AVehicleMaster* Vehicle)
{
    if (bInVehicle && Vehicle == CurrentVehicle)
    {
        Execute_OutOfVehicle(Vehicle, this);
        
        bInVehicle = false;
        CurrentVehicle = nullptr;

        SetPromptEnabled(true);
    }
}

void ACustomPlayerController::Server_SwitchViewMode_Implementation()
{
    if (CurrentVehicle)
        Execute_SwitchViewModeVehicle(CurrentVehicle, this);
}

#pragma endregion



// ----------- Proximity Prompt Handling -----------
#pragma region Proximity Prompt Handling

void ACustomPlayerController::Server_SetPromptManager_Implementation(UProximityPromptManagerComponent* NewPromptManager)
{
    PromptManager = NewPromptManager;
}

void ACustomPlayerController::OnPromptInteracted(UProximityPromptComponent* Prompt, APlayerController* PC, UObject* Context)
{
    if (PC == this && Prompt)
    {
        Server_OnPromptInteracted(Context, Prompt);
    }
}

void ACustomPlayerController::Server_OnPromptInteracted_Implementation(UObject* Context, UProximityPromptComponent* Prompt)
{
    if (!bInVehicle)
    {
        if (AVehicleMaster* Vehicle = Cast<AVehicleMaster>(Context))
        {
            EnterInVehicle(Vehicle, Prompt->GetAttachParent());
        }
        else
        {
            // Autre
        }
    }
}

void ACustomPlayerController::SetPromptEnabled(bool bNewEnabled)
{
    if (!PromptManager)
        return;
    
    PromptManager->SetAllPromptsEnabled(bNewEnabled);
}

#pragma endregion