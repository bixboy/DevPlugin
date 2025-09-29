#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AiData.h"
#include "UnitOrderComponent.generated.h"

class UUnitSelectionComponent;
class UUnitFormationComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOrdersDispatchedSignature, const TArray<AActor*>&, AffectedUnits, const FCommandData&, IssuedCommand);

UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UUnitOrderComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UUnitOrderComponent();

    virtual void BeginPlay() override;

    /** Issues a full command payload to the currently selected units. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Orders")
    void IssueOrder(const FCommandData& CommandData);

    /** Convenience function for Blueprints to submit an order with primitive parameters. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Orders")
    void IssueSimpleOrder(ECommandType Type, FVector Location, FRotator Rotation, AActor* Target, float Radius = 0.f);

    /** Updates the combat behaviour of all currently selected units. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Orders")
    void UpdateBehavior(ECombatBehavior NewBehavior);

    /** Replays the last cached formation command when formation settings change. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Orders")
    void ReapplyCachedFormation();

    /** Blueprint delegate fired once the order has been propagated to all units. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Orders")
    FOnOrdersDispatchedSignature OnOrdersDispatched;

protected:
    UFUNCTION(Server, Reliable)
    void ServerIssueOrder(const FCommandData& CommandData);

    UFUNCTION(Server, Reliable)
    void ServerReapplyCachedFormation();

    UFUNCTION(Server, Reliable)
    void ServerUpdateBehavior(ECombatBehavior NewBehavior);

    void DispatchOrder(const FCommandData& CommandData);
    void ApplyFormationToCommands(const FCommandData& BaseCommand, const TArray<AActor*>& SelectedUnits, TArray<FCommandData>& OutCommands) const;
    void ApplyBehaviorToSelection(ECombatBehavior NewBehavior);
    bool ShouldIgnoreTarget(AActor* Unit, const FCommandData& CommandData) const;

protected:
    UPROPERTY()
    TObjectPtr<UUnitSelectionComponent> SelectionComponent;

    UPROPERTY()
    TObjectPtr<UUnitFormationComponent> FormationComponent;

    /** When true the component will request offsets from the formation component for move orders. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Orders")
    bool bApplyFormationToMoveOrders = true;

    /** When true attack commands towards friendly targets will be ignored. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Orders")
    bool bIgnoreFriendlyTargets = true;

    /** Automatically reapply the cached formation command when spacing/type changes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Orders")
    bool bAutoReapplyCachedFormation = true;
};

