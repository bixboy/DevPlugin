#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PromptWidget.generated.h"

class UTextBlock;


UCLASS()
class STELLARLOCOMOTIONVEHICLESMODULE_API UPromptWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Prompt")
	void SetPromptText(const FText& InText);

protected:
	UPROPERTY(meta=(BindWidget))
	UTextBlock* PromptTextBlock;
};
