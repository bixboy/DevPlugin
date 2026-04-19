#pragma once

#include "CoreMinimal.h"
#include "ContextMenuData.generated.h"

DECLARE_DELEGATE(FContextMenuItemAction);

USTRUCT(BlueprintType)
struct FContextMenuItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText Label;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText Tooltip;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UTexture2D* Icon = nullptr;

    FContextMenuItemAction Action;

    FContextMenuItem() {}
    FContextMenuItem(const FText& InLabel, const FText& InTooltip, UTexture2D* InIcon, FContextMenuItemAction InAction)
        : Label(InLabel), Tooltip(InTooltip), Icon(InIcon), Action(InAction) {}
};
