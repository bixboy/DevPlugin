#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RtsResourceSystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FRTSResourceChangedSignature, AController*, OwningController, FName, ResourceId, int32, NewAmount, int32, NewCapacity);

/** Single resource entry for a player. */
USTRUCT(BlueprintType)
struct FRTSResourceEntry
{
    GENERATED_BODY()

    /** Identifier used inside the economy system (Gold, Wood, Food, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    FName ResourceId = NAME_None;

    /** Current amount owned by the player. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    int32 Amount = 0;

    /** Maximal capacity. Negative values mean unlimited. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    int32 Capacity = -1;

    /** Optional metadata for designers. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    FString DebugDescription;

    bool CanConsume(int32 RequestedAmount) const
    {
        return RequestedAmount <= Amount;
    }

    void AddAmount(int32 Delta)
    {
        Amount = FMath::Clamp(Amount + Delta, 0, Capacity < 0 ? TNumericLimits<int32>::Max() : Capacity);
    }
};

/** World subsystem storing resource counts per controller. */
UCLASS()
class JUPITERPLUGIN_API URTSResourceSystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "RTS|Resources")
    int32 GetResourceAmount(AController* OwningController, FName ResourceId) const;

    UFUNCTION(BlueprintCallable, Category = "RTS|Resources")
    int32 GetResourceCapacity(AController* OwningController, FName ResourceId) const;

    UFUNCTION(BlueprintCallable, Category = "RTS|Resources")
    void AddResource(AController* OwningController, FName ResourceId, int32 DeltaAmount, int32 OptionalNewCapacity = -1);

    UFUNCTION(BlueprintCallable, Category = "RTS|Resources")
    bool ConsumeResource(AController* OwningController, FName ResourceId, int32 AmountToConsume);

    UFUNCTION(BlueprintCallable, Category = "RTS|Resources")
    void ResetResources(AController* OwningController);

    UFUNCTION(BlueprintCallable, Category = "RTS|Resources")
    TArray<FRTSResourceEntry> GetAllResources(AController* OwningController) const;

    /** Event fired whenever a resource amount or capacity changes. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Resources")
    FRTSResourceChangedSignature OnResourceChanged;

private:
    FRTSResourceEntry& FindOrAddEntry(AController* OwningController, FName ResourceId);

    const FRTSResourceEntry* FindEntry(AController* OwningController, FName ResourceId) const;

    /** Map per controller of all resources. */
    TMap<TWeakObjectPtr<AController>, TMap<FName, FRTSResourceEntry>> PlayerResources;
};

