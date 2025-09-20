#pragma once
#include "CoreMinimal.h"
#include "Widget/CustomButtonWidget.h"
#include "Data/AiData.h"
#include "FormationButtonWidget.generated.h"

UCLASS()
class JUPITERPLUGIN_API UFormationButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TEnumAsByte<EFormation> Formation;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UCustomButtonWidget* Button;
};
