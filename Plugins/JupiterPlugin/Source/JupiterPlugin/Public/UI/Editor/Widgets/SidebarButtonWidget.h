#pragma once
#include "CoreMinimal.h"
#include "UI/CustomButtonWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "SidebarButtonWidget.generated.h"


UCLASS()
class JUPITERPLUGIN_API USidebarButtonWidget : public UCustomButtonWidget
{
	GENERATED_BODY()

public:
	virtual void UpdateButtonVisuals(bool bForceStateUpdate = false) override;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* SelectionIndicator;

	// --- Content Colors ---
	UPROPERTY(EditAnywhere, Category = "Jupiter UI | Sidebar Style")
	FLinearColor NormalContentColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Jupiter UI | Sidebar Style")
	FLinearColor SelectedContentColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Jupiter UI | Sidebar Style")
	FLinearColor HoverContentColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Jupiter UI | Sidebar Style")
	FLinearColor SelectionIndicatorColor = FLinearColor(0.0f, 0.45f, 1.0f, 1.0f);
};
