#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Notification/NotificationData.h"
#include "NotificationContainerWidget.generated.h"

class UNotificationEntryWidget;
class UVerticalBox;

USTRUCT()
struct FNotificationWidgetPool
{
	GENERATED_BODY()
    
	UPROPERTY()
	TArray<UNotificationEntryWidget*> Widgets;
};


UCLASS(Abstract)
class JUPITERPLUGIN_API UNotificationContainerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 MaxNotifications = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TSubclassOf<UNotificationEntryWidget> NotificationClass;

protected:
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* NotificationList;

	UPROPERTY()
	TArray<UNotificationEntryWidget*> ActiveWidgets;

	UPROPERTY()
	TMap<UClass*, FNotificationWidgetPool> InactivePools;

	UFUNCTION()
	void OnNotificationAdded(const FNotificationData& Data);
	
	UFUNCTION()
	void OnClearAll(const FNotificationData& Data);

	void RecycleWidget(UNotificationEntryWidget* Widget);
	
	UNotificationEntryWidget* GetWidgetFromPool(TSubclassOf<UNotificationEntryWidget> RequestedClass);
};
