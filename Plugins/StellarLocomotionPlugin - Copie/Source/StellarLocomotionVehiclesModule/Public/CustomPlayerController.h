#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interface/VehiclesInteractions.h"
#include "CustomPlayerController.generated.h"

class UProximityPromptComponent;
class UProximityPromptManagerComponent;
class AVehicleMaster;
class UInputMappingContext;

UCLASS()
class STELLARLOCOMOTIONVEHICLESMODULE_API ACustomPlayerController : public APlayerController, public IVehiclesInteractions
{
	GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(Client, Reliable)
    void Client_AddMappingContext(const UInputMappingContext* MappingContext);

    UFUNCTION(Client, Reliable)
    void Client_RemoveMappingContext(const UInputMappingContext* MappingContext);

	
	UFUNCTION(BlueprintCallable)
    void EnterInVehicle(AVehicleMaster* Vehicle, USceneComponent* ChosenSeat);

	UFUNCTION(BlueprintCallable)
    void ExitVehicle();

	UFUNCTION(BlueprintCallable)
	void SwitchViewMode();

	UFUNCTION(BlueprintCallable)
	void SetPromptEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintPure)
	AVehicleMaster* IsInVehicle() const { return CurrentVehicle; }

protected:
    // ----------- Variables -----------
    UPROPERTY(Replicated)
    bool bInVehicle = false;

    UPROPERTY(Replicated)
    AVehicleMaster* CurrentVehicle = nullptr;

    UPROPERTY(Replicated)
    ACameraVehicle* CurrentCamera = nullptr;
	

    // ----------- Ref ------------
    UPROPERTY()
    UProximityPromptManagerComponent* PromptManager = nullptr;
	

    // ----------- Callbacks ------------
    UFUNCTION()
    void OnPromptInteracted(UProximityPromptComponent* Prompt, APlayerController* PC, UObject* Context);

    UFUNCTION(Server, Reliable)
    void Server_OnPromptInteracted(UObject* Context, UProximityPromptComponent* Prompt);

    UFUNCTION(Server, Reliable)
    void Server_EnterInVehicle(AVehicleMaster* Vehicle, USceneComponent* ChosenSeat);

    UFUNCTION(Server, Reliable)
    void Server_ExitVehicle(AVehicleMaster* Vehicle);

	UFUNCTION(Server, Reliable)
	void Server_SwitchViewMode();

	UFUNCTION(Server, Reliable)
	void Server_SetPromptManager(UProximityPromptManagerComponent* NewPromptManager);
	
};
