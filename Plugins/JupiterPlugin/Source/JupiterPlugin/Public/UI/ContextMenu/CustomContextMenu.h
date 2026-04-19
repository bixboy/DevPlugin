#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ContextMenuData.h"
#include "CustomContextMenu.generated.h"

class UCustomButtonWidget;
class UPanelWidget;


UCLASS(Abstract)
class JUPITERPLUGIN_API UCustomContextMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

	void BuildMenu(const TArray<FContextMenuItem>& Items);

	UFUNCTION(BlueprintCallable, Category = "Context Menu")
	void HideMenu();

protected:
	UPROPERTY(meta = (BindWidget))
	UPanelWidget* MenuContainer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Menu")
	TSubclassOf<UCustomButtonWidget> ButtonClass;

	UFUNCTION()
	void OnOptionClicked(UCustomButtonWidget* Button, int32 Index);

private:
	TArray<FContextMenuItem> CurrentItems;
};
