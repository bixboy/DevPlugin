#pragma once

#include "Components/ActorComponent.h"
#include "FlowFieldTypes.h"
#include "UnitFlowComponent.generated.h"

class ADynamicFlowFieldManager;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ULTRAFLOWFIELDRUNTIME_API UUnitFlowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UUnitFlowComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "UltraFlowField")
    FVector SampleDirection() const;

    UFUNCTION(BlueprintCallable, Category = "UltraFlowField")
    void SetPreferredLayer(EAgentLayer InLayer);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UltraFlowField")
    TSoftObjectPtr<ADynamicFlowFieldManager> FlowFieldManager;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UltraFlowField")
    EAgentLayer PreferredLayer = EAgentLayer::Ground;

private:
    TWeakObjectPtr<ADynamicFlowFieldManager> CachedManager;
    void ResolveManager();
};

