#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "Data/AiData.h"
#include "RtsOrderSystem.generated.h"

class URTSUnitSelectionSubsystem;

/** High level order types exposed to both C++ and Blueprint users. */
UENUM(BlueprintType)
enum class ERTSOrderType : uint8
{
    Move        UMETA(DisplayName = "Move"),
    MoveFast    UMETA(DisplayName = "Move Fast"),
    MoveSlow    UMETA(DisplayName = "Move Slow"),
    Attack      UMETA(DisplayName = "Attack"),
    Patrol      UMETA(DisplayName = "Patrol"),
    Harvest     UMETA(DisplayName = "Harvest"),
    Stop        UMETA(DisplayName = "Stop"),
    Hold        UMETA(DisplayName = "Hold Position"),
    Custom      UMETA(DisplayName = "Custom"),
};

/**
 * Order request descriptor. Designers can easily spawn orders from Blueprints
 * while programmers retain access to the underlying legacy FCommandData.
 */
USTRUCT(BlueprintType)
struct FRTSOrderRequest
{
    GENERATED_BODY()

    /** Type of order to execute. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    ERTSOrderType OrderType = ERTSOrderType::Move;

    /** Desired target location. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    FVector TargetLocation = FVector::ZeroVector;

    /** Optional target actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    TObjectPtr<AActor> TargetActor = nullptr;

    /** Optional radius used for formation spreads or area abilities. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    float Radius = 0.f;

    /** Gameplay tags attached to the order (useful for filtering or priority rules). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    FGameplayTagContainer OrderTags;

    /** Should the order be queued instead of replacing the current one. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    bool bQueue = false;

    /** Legacy command used by the existing code base. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    FCommandData LegacyCommand;

    FRTSOrderRequest() = default;

    explicit FRTSOrderRequest(const FCommandData& CommandData)
        : OrderType(ERTSOrderType::Move)
        , TargetLocation(CommandData.Location)
        , TargetActor(CommandData.Target)
        , Radius(CommandData.Radius)
        , LegacyCommand(CommandData)
    {
        OrderType = FromCommandType(CommandData.Type);
    }

    /** Build an FCommandData from the request for backward compatibility. */
    FCommandData ToCommandData(AController* RequestingController) const;

    /** Helper converting the enum between legacy and new representation. */
    static ERTSOrderType FromCommandType(ECommandType CommandType);
    static ECommandType ToCommandType(ERTSOrderType OrderType);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FRTSOrderIssuedSignature, AController*, IssuingController, const TArray<AActor*>&, Units, const FRTSOrderRequest&, OrderRequest, bool, bQueued, bool, bLocalExecution);

/** Subsystem responsible for dispatching orders to units in a network friendly way. */
UCLASS()
class JUPITERPLUGIN_API URTSOrderSystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** High level API used by Blueprints or external code. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Orders")
    void IssueOrder(AController* IssuingController, const TArray<AActor*>& Units, const FRTSOrderRequest& OrderRequest);

    /** Compatibility helper used by legacy systems relying on FCommandData directly. */
    void IssueOrder(AController* IssuingController, const TArray<AActor*>& Units, const FCommandData& CommandData, bool bQueueCommand);

    /** Broadcast after an order has been successfully dispatched. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Orders")
    FRTSOrderIssuedSignature OnOrderIssued;

protected:
    void DispatchOrderToUnit(AController* IssuingController, AActor* Unit, const FRTSOrderRequest& OrderRequest) const;

    /** Returns whether the issuing controller locally controls the unit (useful for prediction). */
    static bool IsLocalExecution(AController* IssuingController, AActor* Unit);
};

