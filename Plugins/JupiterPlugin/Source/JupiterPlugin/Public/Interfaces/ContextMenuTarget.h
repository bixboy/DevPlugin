#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UI/ContextMenuData.h"
#include "ContextMenuTarget.generated.h"


UINTERFACE(MinimalAPI, Blueprintable)
class UContextMenuTarget : public UInterface
{
    GENERATED_BODY()
};

class JUPITERPLUGIN_API IContextMenuTarget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI|ContextMenu")
    void GetContextMenuOptions(TArray<FContextMenuItem>& OutOptions);
};
