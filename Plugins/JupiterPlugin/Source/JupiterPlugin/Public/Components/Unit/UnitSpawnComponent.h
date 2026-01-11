#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitSpawnComponent.generated.h"

class UUnitSelectionComponent;
class ASoldierRts;

UENUM(BlueprintType)
enum class ESpawnFormation : uint8
{
    Square,
    Line,
    Column,
    Wedge,
    Blob,
    Custom
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnUnitClassChangedSignature, TSubclassOf<ASoldierRts>, NewUnitClass);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnCountChangedSignature, int32, NewCount);
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

    // --- Public API (Blueprint Callable) ---
	
    UFUNCTION(BlueprintCallable) void SetUnitToSpawn(TSubclassOf<ASoldierRts> NewUnit);
    UFUNCTION(BlueprintCallable) void SetUnitsPerSpawn(int32 NewCount);
    UFUNCTION(BlueprintCallable) void SetSpawnFormation(ESpawnFormation NewFormation);
    UFUNCTION(BlueprintCallable) void SetCustomFormationDimensions(FIntPoint NewDimensions);

    // Commandes d'action
	
    UFUNCTION(BlueprintCallable) void SpawnUnits();
    UFUNCTION(BlueprintCallable) void SpawnUnitsWithTransform(const FVector& Location, const FRotator& Rotation);

    // Getters & Utils
	
    UFUNCTION(BlueprintPure) int32 GetUnitsPerSpawn() const { return UnitsPerSpawn; }
    UFUNCTION(BlueprintPure) TSubclassOf<ASoldierRts> GetUnitToSpawn() const { return UnitToSpawn; }
    UFUNCTION(BlueprintPure) float GetFormationSpacing() const { return FormationSpacing; }
    UFUNCTION(BlueprintPure) ESpawnFormation GetSpawnFormation() const { return SpawnFormation; }
    FIntPoint GetCustomFormationDimensions() const { return CustomFormationDimensions; }

    UFUNCTION(BlueprintCallable)
    void BuildSpawnFormationOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets, const FRotator& Facing) const;

public:
    // Events
    UPROPERTY(BlueprintAssignable) FOnSpawnUnitClassChangedSignature OnUnitClassChanged;
    UPROPERTY(BlueprintAssignable) FOnSpawnCountChangedSignature OnSpawnCountChanged;
    UPROPERTY(BlueprintAssignable) FOnSpawnFormationChangedSignature OnSpawnFormationChanged;
    UPROPERTY(BlueprintAssignable) FOnCustomFormationDimensionsChangedSignature OnCustomFormationDimensionsChanged;

protected:
    // Server RPCs
    UFUNCTION(Server, Reliable) void ServerSetUnitClass(TSubclassOf<ASoldierRts> NewUnit);
    UFUNCTION(Server, Reliable) void ServerSpawnUnits(const FVector& Location, const FRotator& Rotation);
    UFUNCTION(Server, Reliable) void ServerSetUnitsPerSpawn(int32 NewCount);
    UFUNCTION(Server, Reliable) void ServerSetSpawnFormation(ESpawnFormation NewFormation);
    UFUNCTION(Server, Reliable) void ServerSetCustomFormationDimensions(FIntPoint NewDimensions);

    // Rep notifies
    UFUNCTION() void OnRep_UnitToSpawn();
    UFUNCTION() void OnRep_UnitsPerSpawn();
    UFUNCTION() void OnRep_SpawnFormation();
    UFUNCTION() void OnRep_CustomFormationDimensions();

    // Helpers
    void UpdateSpawnCountFromCustomFormation();
    void GenerateFormationOffsets(TArray<FVector>& OutOffsets, int32 Count, float Spacing) const;

protected:
    UPROPERTY() TObjectPtr<UUnitSelectionComponent> SelectionComponent;

    UPROPERTY(EditAnywhere) bool bRequireGroundHit = true;
    UPROPERTY(EditAnywhere) float SpawnTraceTolerance = 50.f;
    UPROPERTY(EditAnywhere) bool bNotifyOnServerOnly = false;

    UPROPERTY(EditAnywhere) TSubclassOf<ASoldierRts> DefaultUnitClass;

    UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_UnitsPerSpawn, meta=(ClampMin=1))
    int32 UnitsPerSpawn = 1;

    UPROPERTY(EditAnywhere, meta=(ClampMin=0))
    float FormationSpacing = 150.f;

    UPROPERTY(ReplicatedUsing = OnRep_UnitToSpawn)
    TSubclassOf<ASoldierRts> UnitToSpawn;

    UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_SpawnFormation)
    ESpawnFormation SpawnFormation = ESpawnFormation::Square;

    UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_CustomFormationDimensions, meta=(ClampMin=1))
    FIntPoint CustomFormationDimensions = FIntPoint(3, 3);
};