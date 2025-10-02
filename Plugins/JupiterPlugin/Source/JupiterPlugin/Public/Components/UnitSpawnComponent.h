#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitSpawnComponent.generated.h"

class UUnitSelectionComponent;
class ASoldierRts;

UENUM(BlueprintType)
enum class ESpawnFormation : uint8
{
    Square     UMETA(DisplayName = "Square"),
    Line       UMETA(DisplayName = "Line"),
    Column     UMETA(DisplayName = "Column"),
    Wedge      UMETA(DisplayName = "Wedge"),
    Blob       UMETA(DisplayName = "Blob"),
    Custom     UMETA(DisplayName = "Custom")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnUnitClassChangedSignature, TSubclassOf<ASoldierRts>, NewUnitClass);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnCountChangedSignature, int32, NewSpawnCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnFormationChangedSignature, ESpawnFormation, NewFormation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCustomFormationDimensionsChangedSignature, FIntPoint, NewDimensions);

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

    UFUNCTION(BlueprintCallable, Category = "RTS|Spawn")
    void SetSpawnFormation(ESpawnFormation NewFormation);

    UFUNCTION(BlueprintPure, Category = "RTS|Spawn")
    ESpawnFormation GetSpawnFormation() const { return SpawnFormation; }

    UFUNCTION(BlueprintCallable, Category = "RTS|Spawn")
    void SetCustomFormationDimensions(FIntPoint NewDimensions);

    UFUNCTION(BlueprintPure, Category = "RTS|Spawn")
    FIntPoint GetCustomFormationDimensions() const { return CustomFormationDimensions; }

    /** Delegate fired whenever the unit class changes (useful for UI preview). */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Spawn")
    FOnSpawnUnitClassChangedSignature OnUnitClassChanged;

    /** Delegate fired whenever the spawn count changes. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Spawn")
    FOnSpawnCountChangedSignature OnSpawnCountChanged;

    /** Delegate fired whenever the spawn formation changes. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Spawn")
    FOnSpawnFormationChangedSignature OnSpawnFormationChanged;

    /** Delegate fired whenever the custom formation dimensions change. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Spawn")
    FOnCustomFormationDimensionsChangedSignature OnCustomFormationDimensionsChanged;

protected:
    UFUNCTION(Server, Reliable)
    void ServerSetUnitClass(TSubclassOf<ASoldierRts> NewUnitClass);

    UFUNCTION(Server, Reliable)
    void ServerSpawnUnits(const FVector& SpawnLocation);

    UFUNCTION(Server, Reliable)
    void ServerSetUnitsPerSpawn(int32 NewSpawnCount);

    UFUNCTION(Server, Reliable)
    void ServerSetSpawnFormation(ESpawnFormation NewFormation);

    UFUNCTION(Server, Reliable)
    void ServerSetCustomFormationDimensions(FIntPoint NewDimensions);

    UFUNCTION()
    void OnRep_UnitToSpawn();

    UFUNCTION()
    void OnRep_UnitsPerSpawn();

    UFUNCTION()
    void OnRep_SpawnFormation();

    UFUNCTION()
    void OnRep_CustomFormationDimensions();

    void GenerateSpawnOffsets(TArray<FVector>& OutOffsets, int32 SpawnCount, float Spacing) const;

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

    /** Formation layout used when spawning batches of units. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_SpawnFormation, Category = "RTS|Spawn")
    TEnumAsByte<ESpawnFormation> SpawnFormation = ESpawnFormation::Square;

    /** Custom grid dimensions (X = columns, Y = rows) used when SpawnFormation is Custom. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_CustomFormationDimensions, Category = "RTS|Spawn", meta = (ClampMin = "1"))
    FIntPoint CustomFormationDimensions = FIntPoint(3, 3);
};

