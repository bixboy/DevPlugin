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

    UFUNCTION(BlueprintCallable, Category = "RTS|Orders")
    void IssueOrder(const FCommandData& CommandData);

    UFUNCTION(BlueprintCallable, Category = "RTS|Orders")
    void IssueSimpleOrder(ECommandType Type, FVector Location, FRotator Rotation, AActor* Target, float Radius = 0.f);

    UFUNCTION(BlueprintCallable, Category = "RTS|Orders")
    void UpdateBehavior(ECombatBehavior NewBehavior);

    UFUNCTION(BlueprintCallable, Category = "RTS|Orders")
    void ReapplyCachedFormation();

    UPROPERTY(BlueprintAssignable, Category = "RTS|Orders")
    FOnOrdersDispatchedSignature OnOrdersDispatched;

protected:
    // --- Networking ---
	
    UFUNCTION(Server, Reliable)
    void ServerIssueOrder(const FCommandData& CommandData, const TArray<AActor*>& UnitsToCommand);

    UFUNCTION(Server, Reliable)
    void ServerUpdateBehavior(ECombatBehavior NewBehavior, const TArray<AActor*>& UnitsToCommand);

    UFUNCTION(Server, Reliable)
    void ServerReapplyCachedFormation(const TArray<AActor*>& UnitsToCommand);

    // --- Logic ---
	
    void DispatchOrder(const FCommandData& CommandData, const TArray<AActor*>& Units);
    
    bool PreparePatrolCommand(FCommandData& InOutCommand, const TArray<AActor*>& Units);

    void ApplyFormationToCommands(const FCommandData& BaseCommand, const TArray<AActor*>& SelectedUnits, TArray<FCommandData>& OutCommands) const;
    void ApplyBehaviorToSelection(ECombatBehavior NewBehavior, const TArray<AActor*>& Units);
    bool ShouldIgnoreTarget(AActor* Unit, const FCommandData& CommandData) const;

protected:
    UPROPERTY()
    TObjectPtr<UUnitSelectionComponent> SelectionComponent;

    UPROPERTY()
    TObjectPtr<UUnitFormationComponent> FormationComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Orders")
    bool bApplyFormationToMoveOrders = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Orders")
    bool bIgnoreFriendlyTargets = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Orders")
    bool bAutoReapplyCachedFormation = true;
};