#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/Notification/NotificationData.h"
#include "NotificationSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNotificationAdded, const FNotificationData&, Data);

UCLASS()
class JUPITERPLUGIN_API UNotificationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- Public API ---
	UFUNCTION(BlueprintCallable, Category = "Notification")
	void AddNotification(const FNotificationData& Data);

	UFUNCTION(BlueprintCallable, Category = "Notification")
	void ClearAll();

    UFUNCTION(BlueprintCallable, Category = "Notification")
	void NativeNotify(const FText& Title, const FText& Message, FLinearColor Color = FLinearColor::White, float Duration = 5.0f);

	UPROPERTY(BlueprintAssignable, Category = "Notification")
	FOnNotificationAdded OnNotificationAdded;

    UPROPERTY(BlueprintAssignable, Category = "Notification")
    FOnNotificationAdded OnClearAll;
};
