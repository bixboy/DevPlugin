#pragma once

#include "Components/ActorComponent.h"
#include "LocalAvoidanceComponent.generated.h"

USTRUCT(BlueprintType)
struct FLocalAvoidanceSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float QueryRadius = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float MaxForce = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float UpdateInterval = 0.1f;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ULTRAFLOWFIELDRUNTIME_API ULocalAvoidanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULocalAvoidanceComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Avoidance")
    FVector GetAvoidanceVector() const { return CachedAvoidance; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    FLocalAvoidanceSettings Settings;

private:
    FVector CachedAvoidance = FVector::ZeroVector;
    float TimeAccumulator = 0.f;

    void UpdateAvoidance();
};

