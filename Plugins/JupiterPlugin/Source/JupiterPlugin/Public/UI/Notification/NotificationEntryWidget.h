#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Notification/NotificationData.h"
#include "NotificationEntryWidget.generated.h"

class UTextBlock;
class UImage;
class UCustomButtonWidget;

UCLASS(Abstract)
class JUPITERPLUGIN_API UNotificationEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Notification")
	void SetupNotification(const FNotificationData& Data);

	virtual void SetupNotification_Implementation(const FNotificationData& Data);

	UFUNCTION(BlueprintCallable, Category = "Notification")
	void ResetState();

	FSimpleDelegate OnFinished;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_Title;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_SubTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* Image_Icon;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* Image_AccentStrip;
	
	UPROPERTY(meta = (BindWidgetOptional))
	UCustomButtonWidget* Btn_Action;
	
	FNotificationData CurrentData;
	
	FTimerHandle DismissTimerHandle;
	
	UFUNCTION()
	void OnActionClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION(BlueprintImplementableEvent, Category = "Notification")
	void PlayIntroAnimation();

	UFUNCTION(BlueprintImplementableEvent, Category = "Notification")
	void PlayOutroAnimation();

	UFUNCTION(BlueprintCallable, Category = "Notification")
	void NotifyAnimationFinished();
	
	UFUNCTION()
	void StartDismissTimer(float Duration);
	
	UFUNCTION()
	void HandleDismissTimer();
};
