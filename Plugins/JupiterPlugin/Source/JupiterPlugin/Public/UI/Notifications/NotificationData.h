#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "NotificationData.generated.h"

DECLARE_DYNAMIC_DELEGATE(FNotificationActionDelegate);

USTRUCT(BlueprintType)
struct FNotificationData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Title;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText SubTitle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLinearColor AccentColor = FLinearColor::White;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Duration = 5.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FNotificationActionDelegate Action;

    // C++ Only Action (supporting Lambdas)
    FSimpleDelegate NativeAction;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName Tag;

    // Optional: Override the visual widget class for this notification
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<UUserWidget> VisualClass;
};
