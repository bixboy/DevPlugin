#include "Subsystems/TooltipSubsystem.h"


void UTooltipSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bIsTooltipVisible = false;
}

void UTooltipSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UTooltipSubsystem::ShowTooltip(const FTooltipData& Data)
{
    bIsTooltipVisible = true;
    OnShowTooltip.Broadcast(Data);
}

void UTooltipSubsystem::HideTooltip()
{
    if (bIsTooltipVisible)
    {
        bIsTooltipVisible = false;
        OnHideTooltip.Broadcast();
    }
}
