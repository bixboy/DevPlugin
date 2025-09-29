#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitSpawnComponent.generated.h"

class UUnitSelectionComponent;
class ASoldierRts;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnUnitClassChangedSignature, TSubclassOf<ASoldierRts>, NewUnitClass);

UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UUnitSpawnComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UUnitSpawnComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Updates the unit class used when spawning new units. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Spawn")
    void SetUnitToSpawn(TSubclassOf<ASoldierRts> NewUnitClass);

    /** Spawns the currently selected unit class at the mouse cursor location. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Spawn")
    void SpawnUnits();

    UFUNCTION(BlueprintPure, Category = "RTS|Spawn")
    TSubclassOf<ASoldierRts> GetUnitToSpawn() const { return UnitToSpawn; }

    /** Delegate fired whenever the unit class changes (useful for UI preview). */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Spawn")
    FOnSpawnUnitClassChangedSignature OnUnitClassChanged;

protected:
    UFUNCTION(Server, Reliable)
    void ServerSetUnitClass(TSubclassOf<ASoldierRts> NewUnitClass);

    UFUNCTION(Server, Reliable)
    void ServerSpawnUnits(const FVector& SpawnLocation);

    UFUNCTION()
    void OnRep_UnitToSpawn();

protected:
    UPROPERTY()
    TObjectPtr<UUnitSelectionComponent> SelectionComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
    bool bRequireGroundHit = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
    float SpawnTraceTolerance = 50.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
    bool bNotifyOnServerOnly = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
    TSubclassOf<ASoldierRts> DefaultUnitClass;

    UPROPERTY(ReplicatedUsing = OnRep_UnitToSpawn)
    TSubclassOf<ASoldierRts> UnitToSpawn;
};

