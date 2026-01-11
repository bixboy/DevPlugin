#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AiData.h"
#include "CommandComponent.generated.h"

class ASoldierRts;
class AAiControllerRts;
class UCharacterMovementComponent;


#define ECC_Terrain ECC_GameTraceChannel1


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UCommandComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCommandComponent();
    
    virtual void BeginPlay() override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // --- API ---
	
    UFUNCTION(BlueprintCallable, Category = "RTS|Command")
    void CommandMoveToLocation(const FCommandData& CommandData);
    
    UFUNCTION(BlueprintCallable, Category = "RTS|Command")
    void SetOwnerAIController(AAiControllerRts* Controller);

    UFUNCTION(BlueprintPure, Category = "RTS|Command")
    FVector GetCommandLocation() const { return TargetLocation; }

    UFUNCTION(BlueprintPure, Category = "RTS|Command")
    FCommandData GetCurrentCommand() const { return CurrentCommand; }

protected:
    void InitializeMovementComponent() const;

    // --- Internal Logic ---
	
    void CommandPatrol(const FCommandData& CommandData);
    void CommandMove(const FCommandData& CommandData);
    
    UFUNCTION()
    void OnDestinationReached(const FCommandData CommandData);

    // Helpers
	
    bool ShouldFollowCommandTarget(const FCommandData& CommandData) const;
    bool HasValidAttackTarget(const FCommandData& CommandData) const;
    FVector ResolveDestinationFromCommand(const FCommandData& CommandData) const;
    void ApplyMovementSettings(const FCommandData& CommandData);

    // Speed modifiers
	
    void SetWalk() const;
    void SetRun() const;
    void SetSprint() const;

    // Orientation Logic
	
    void UpdateOrientation(float DeltaTime);
    bool IsOriented() const;

    // Marker Logic
	
    FTransform GetMarkerTransformOnGround(const FVector& Position) const;
    void CreateMoveMarker();

    // --- Networking : Visual Feedback ---

    UFUNCTION(Client, Reliable)
    void Client_UpdateMoveMarker(const FVector& Location, const FCommandData& CommandData);

    UFUNCTION(Client, Reliable)
    void Client_ShowMoveMarker(bool bShow);

protected:
    // --- Data ---
	
    UPROPERTY(Replicated)
    FCommandData CurrentCommand;

    UPROPERTY()
    TObjectPtr<ASoldierRts> OwnerActor;
    
    UPROPERTY()
    TObjectPtr<UCharacterMovementComponent> OwnerCharaMovementComp;
    
    UPROPERTY()
    TObjectPtr<AAiControllerRts> OwnerAIController;

    // Settings
	
    UPROPERTY(EditAnywhere, Category = "Command|Settings")
    float MaxSpeed = 600.f;

    UPROPERTY(EditAnywhere, Category = "Command|Visuals")
    TSubclassOf<AActor> MoveMarkerClass;

    // State
    UPROPERTY()
    TObjectPtr<AActor> MoveMarker;

    UPROPERTY()
    FVector TargetLocation;

    UPROPERTY()
    FRotator TargetOrientation;

    bool bShouldOrientate = false;
    bool bHaveTargetAttack = false;
};