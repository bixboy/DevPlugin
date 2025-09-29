#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitSpawnComponent.generated.h"

class UUnitSelectionComponent;
class ASoldierRts;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnUnitClassChangedSignature, TSubclassOf<ASoldierRts>, NewUnitClass);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnCountChangedSignature, int32, NewSpawnCount);

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

    /** Updates how many units should be spawned at once. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Spawn")
    void SetUnitsPerSpawn(int32 NewSpawnCount);

    UFUNCTION(BlueprintPure, Category = "RTS|Spawn")
    int32 GetUnitsPerSpawn() const { return UnitsPerSpawn; }

    UFUNCTION(BlueprintPure, Category = "RTS|Spawn")
    TSubclassOf<ASoldierRts> GetUnitToSpawn() const { return UnitToSpawn; }

    UFUNCTION(BlueprintPure, Category = "RTS|Spawn")
    float GetFormationSpacing() const { return FormationSpacing; }

    /** Delegate fired whenever the unit class changes (useful for UI preview). */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Spawn")
    FOnSpawnUnitClassChangedSignature OnUnitClassChanged;

    /** Delegate fired whenever the spawn count changes. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Spawn")
    FOnSpawnCountChangedSignature OnSpawnCountChanged;

protected:
    UFUNCTION(Server, Reliable)
    void ServerSetUnitClass(TSubclassOf<ASoldierRts> NewUnitClass);

    UFUNCTION(Server, Reliable)
    void ServerSpawnUnits(const FVector& SpawnLocation);

    UFUNCTION(Server, Reliable)
    void ServerSetUnitsPerSpawn(int32 NewSpawnCount);

    UFUNCTION()
    void OnRep_UnitToSpawn();

    UFUNCTION()
    void OnRep_UnitsPerSpawn();

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

    /** Number of units spawned at once. Clamped to be at least 1. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_UnitsPerSpawn, Category = "RTS|Spawn", meta = (ClampMin = "1"))
    int32 UnitsPerSpawn = 1;

    /** Distance between units when spawning in formation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn", meta = (ClampMin = "0"))
    float FormationSpacing = 150.f;

    UPROPERTY(ReplicatedUsing = OnRep_UnitToSpawn)
    TSubclassOf<ASoldierRts> UnitToSpawn;
};

