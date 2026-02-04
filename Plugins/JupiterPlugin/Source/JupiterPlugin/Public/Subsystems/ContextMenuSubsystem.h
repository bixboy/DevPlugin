#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/ContextMenuData.h"
#include "ContextMenuSubsystem.generated.h"

class UCustomContextMenu;

UCLASS()
class JUPITERPLUGIN_API UContextMenuSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    
    UFUNCTION(BlueprintCallable, Category = "UI|ContextMenu")
    void ShowContextMenu(const TArray<FContextMenuItem>& Options, const FVector2D& ScreenPosition);

    UFUNCTION(BlueprintCallable, Category = "UI|ContextMenu")
    void HideContextMenu();

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UCustomContextMenu> MenuWidgetClass;

private:
    UPROPERTY()
    TObjectPtr<UCustomContextMenu> ActiveMenuWidget;
};
