#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlowFieldScheduler.h"
#include "DynamicFlowFieldManager.generated.h"

class UFlowVoxelGridComponent;
class UBillboardComponent;

UCLASS()
class ULTRAFLOWFIELDRUNTIME_API ADynamicFlowFieldManager : public AActor
{
    GENERATED_BODY()

public:
    ADynamicFlowFieldManager();

    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "UltraFlowField")
    void SetGlobalTarget(const FVector& Target);

    UFUNCTION(BlueprintCallable, Category = "UltraFlowField")
    void SetAgentLayer(EAgentLayer InLayer);

    UFUNCTION(BlueprintCallable, Category = "UltraFlowField")
    void MarkObstacleBox(const FVector& Center, const FVector& Extents, bool bBlocked);

    UFUNCTION(BlueprintCallable, Category = "UltraFlowField")
    void SetDebugEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "UltraFlowField")
    bool IsDebugEnabled() const { return bEnableDebugDraw; }

    UFUNCTION(BlueprintCallable, Category = "UltraFlowField")
    FVector GetFlowDirectionAt(const FVector& WorldLocation, EAgentLayer Layer) const;

    UFUNCTION(BlueprintCallable, Category = "UltraFlowField")
    void ToggleThrottle(bool bEnabled);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UltraFlowField")
    TObjectPtr<UFlowVoxelGridComponent> VoxelGrid;

protected:
    UPROPERTY(EditAnywhere, Category = "UltraFlowField")
    bool bEnableDebugDraw = false;

    UPROPERTY(EditAnywhere, Category = "UltraFlowField")
    bool bThrottle = true;

    UPROPERTY(EditAnywhere, Category = "UltraFlowField")
    float MaxPerFrameTimeMs = 2.0f;

private:
    void RequestRebuild();
    void HandleCompletedJobs();

    FVector CurrentTarget = FVector::ZeroVector;
    EAgentLayer ActiveLayer = EAgentLayer::Ground;

    FFlowFieldScheduler Scheduler;

    mutable FRWLock BufferLock;
    TMap<FIntVector, TSharedPtr<FFlowFieldBuffer>> FrontBuffers;
    TMap<FIntVector, TSharedPtr<FFlowFieldBuffer>> BackBuffers;

    void SwapBuffers(TSharedPtr<FFlowFieldBuildOutput> Output);

    void DrawDebugInfo();

    UPROPERTY()
    TObjectPtr<UBillboardComponent> SpriteComponent;
};

