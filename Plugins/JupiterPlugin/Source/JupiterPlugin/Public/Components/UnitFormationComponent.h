#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AiData.h"
#include "UnitFormationComponent.generated.h"

class UFormationDataAsset;
class UUnitSelectionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFormationStateChangedSignature);

UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UUnitFormationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UUnitFormationComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Updates the current formation type. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Formation")
    void SetFormation(EFormation NewFormation);

    /** Updates the spacing between units for the active formation. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Formation")
    void SetFormationSpacing(float NewSpacing);

    /** Returns the currently active formation type. */
    UFUNCTION(BlueprintPure, Category = "RTS|Formation")
    EFormation GetFormation() const;

    /** Returns the spacing currently applied to the active formation. */
    UFUNCTION(BlueprintPure, Category = "RTS|Formation")
    float GetFormationSpacing() const;

    /** Returns the cached command used for the previous formation order (nullptr if none). */
    const FCommandData* GetCachedFormationCommand() const;

    /** Generates formation-aware command data for each selected unit. */
    void BuildFormationCommands(const FCommandData& BaseCommand, const TArray<AActor*>& Units, TArray<FCommandData>& OutCommands);

    /** Returns true if more than one unit is selected (proxy to selection component). */
    UFUNCTION(BlueprintPure, Category = "RTS|Formation")
    bool HasGroupSelection() const;

    /** Delegate fired whenever formation spacing or type changes. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Formation")
    FOnFormationStateChangedSignature OnFormationStateChanged;

protected:
    UFUNCTION(Server, Reliable)
    void ServerSetFormation(EFormation NewFormation);

    UFUNCTION(Server, Reliable)
    void ServerSetSpacing(float NewSpacing);

    UFUNCTION()
    void OnRep_Formation();

    UFUNCTION()
    void OnRep_Spacing();

    FVector CalculateOffset(int32 Index, int32 TotalUnits) const;
    void CenterFormationOffsets(TArray<FVector>& Offsets) const;
    void CacheFormationCommand(const FCommandData& CommandData, const TArray<AActor*>& Units);
    void BroadcastFormationChanged();
    UFormationDataAsset* GetFormationData() const;

protected:
    UPROPERTY()
    TObjectPtr<UUnitSelectionComponent> SelectionComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Formation")
    TArray<UFormationDataAsset*> FormationDataAssets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Formation")
    bool bRespectCommandRotation = true;

    /** Keeps the destination of the original command at the center of the generated formation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Formation")
    bool bMaintainFormationCenter = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Formation")
    bool bCacheLastFormationCommand = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Formation")
    float MinimumSpacing = 25.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Formation", ReplicatedUsing = OnRep_Spacing)
    float FormationSpacing = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Formation", ReplicatedUsing = OnRep_Formation)
    TEnumAsByte<EFormation> CurrentFormation = EFormation::Square;

    UPROPERTY()
    FCommandData CachedFormationCommand;

    UPROPERTY()
    bool bHasCachedCommand = false;
};

