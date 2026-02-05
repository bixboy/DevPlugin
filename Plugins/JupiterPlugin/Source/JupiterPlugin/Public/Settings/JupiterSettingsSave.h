#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "JupiterSettingsSave.generated.h"


UCLASS()
class JUPITERPLUGIN_API UJupiterSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:
	UJupiterSettingsSave();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Tooltip")
	float TooltipDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Notification")
	float NotificationDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Camera")
	float CameraSpeed = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Camera")
	float RotateSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Camera")
	float EdgeScrollSpeed = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Camera")
	bool bCanEdgeScroll = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Camera")
	float MinZoom = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Camera")
	float MaxZoom = 4000.0f;
};
