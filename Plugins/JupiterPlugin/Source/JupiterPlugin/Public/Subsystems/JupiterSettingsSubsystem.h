#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Settings/JupiterSettingsSave.h"
#include "JupiterSettingsSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsChanged);


UCLASS()
class JUPITERPLUGIN_API UJupiterSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// --- Save/Load Inteface ---
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadSettings();

	UPROPERTY(BlueprintAssignable, Category = "Settings")
	FOnSettingsChanged OnSettingsChanged;

	// --- Getters/Setters ---

	// Tooltip
	UFUNCTION(BlueprintCallable, Category = "Settings|Tooltip")
	void SetTooltipDelay(float NewValue);

	UFUNCTION(BlueprintPure, Category = "Settings|Tooltip")
	float GetTooltipDelay() const;

	// Notification
	UFUNCTION(BlueprintCallable, Category = "Settings|Notification")
	void SetNotificationDuration(float NewValue);

	UFUNCTION(BlueprintPure, Category = "Settings|Notification")
	float GetNotificationDuration() const;

	// Camera
	UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
	void SetCameraSpeed(float NewValue);

	UFUNCTION(BlueprintPure, Category = "Settings|Camera")
	float GetCameraSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
	void SetRotateSpeed(float NewValue);

	UFUNCTION(BlueprintPure, Category = "Settings|Camera")
	float GetRotateSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
	void SetEdgeScrollSpeed(float NewValue);

	UFUNCTION(BlueprintPure, Category = "Settings|Camera")
	float GetEdgeScrollSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
	void SetCanEdgeScroll(bool bNewValue);

	UFUNCTION(BlueprintPure, Category = "Settings|Camera")
	bool GetCanEdgeScroll() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
	void SetZoomLimits(float NewMin, float NewMax);

	UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
	void GetZoomLimits(float& OutMin, float& OutMax) const;

protected:
	UPROPERTY()
	TObjectPtr<UJupiterSettingsSave> CurrentSettings;

	const FString SaveSlotName = TEXT("JupiterUserSettings");
	const int32 SaveUserIndex = 0;
};
