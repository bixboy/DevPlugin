#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Subsystems/TooltipSubsystem.h"
#include "WorldTooltipTarget.generated.h"


UINTERFACE(MinimalAPI, Blueprintable)
class UWorldTooltipTarget : public UInterface
{
    GENERATED_BODY()
};

class JUPITERPLUGIN_API IWorldTooltipTarget
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI|Tooltip")
    bool GetTooltipData(FTooltipData& OutData);
};
