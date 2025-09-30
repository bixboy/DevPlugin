#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlowFieldSampleActors.generated.h"

class UUnitFlowComponent;
class ULocalAvoidanceComponent;
class UFloatingPawnMovement;

UCLASS()
class ULTRAFLOWFIELDRUNTIME_API AFlowFieldSampleUnit : public APawn
{
    GENERATED_BODY()

public:
    AFlowFieldSampleUnit();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override {}

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UUnitFlowComponent> FlowComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<ULocalAvoidanceComponent> AvoidanceComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UltraFlowField")
    float MoveSpeed = 600.f;

protected:
    virtual void BeginPlay() override;
};

UCLASS()
class ULTRAFLOWFIELDRUNTIME_API AFlowFieldSampleManagerActor : public AActor
{
    GENERATED_BODY()

public:
    AFlowFieldSampleManagerActor();

    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<ADynamicFlowFieldManager> Manager;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UltraFlowField")
    TSubclassOf<AFlowFieldSampleUnit> UnitClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UltraFlowField")
    int32 InitialUnits = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UltraFlowField")
    float SpawnRadius = 2000.f;

    UFUNCTION(BlueprintCallable, Category = "UltraFlowField")
    void SpawnUnits(int32 Count);

    UFUNCTION(BlueprintCallable, Category = "UltraFlowField")
    void SetTargetAtLocation(const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "UltraFlowField")
    void ToggleDebug();

private:
    TArray<TWeakObjectPtr<AFlowFieldSampleUnit>> SpawnedUnits;
};

