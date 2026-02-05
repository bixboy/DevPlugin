#include "Subsystems/JupiterSettingsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Notification/NotificationSubsystem.h"

void UJupiterSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadSettings();
}

void UJupiterSettingsSubsystem::SaveSettings()
{
	if (CurrentSettings)
	{
		UGameplayStatics::AsyncSaveGameToSlot(CurrentSettings, SaveSlotName, SaveUserIndex);
	}
}

void UJupiterSettingsSubsystem::LoadSettings()
{
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		CurrentSettings = Cast<UJupiterSettingsSave>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
	}

	if (!CurrentSettings)
	{
		CurrentSettings = Cast<UJupiterSettingsSave>(UGameplayStatics::CreateSaveGameObject(UJupiterSettingsSave::StaticClass()));
		SaveSettings();
	}

    if (UNotificationSubsystem* NotifSys = GetGameInstance()->GetSubsystem<UNotificationSubsystem>())
    {
        NotifSys->SetDefaultDuration(GetNotificationDuration());
    }
}

// --- Getters/Setters ---

void UJupiterSettingsSubsystem::SetTooltipDelay(float NewValue)
{
	if (CurrentSettings && CurrentSettings->TooltipDelay != NewValue)
	{
		CurrentSettings->TooltipDelay = NewValue;
		OnSettingsChanged.Broadcast();
	}
}

float UJupiterSettingsSubsystem::GetTooltipDelay() const
{
	return CurrentSettings ? CurrentSettings->TooltipDelay : 0.5f;
}

void UJupiterSettingsSubsystem::SetNotificationDuration(float NewValue)
{
	if (CurrentSettings && CurrentSettings->NotificationDuration != NewValue)
	{
		CurrentSettings->NotificationDuration = NewValue;
        
        if (UNotificationSubsystem* NotifSys = GetGameInstance()->GetSubsystem<UNotificationSubsystem>())
        {
            NotifSys->SetDefaultDuration(NewValue);
        }

		OnSettingsChanged.Broadcast();
	}
}

float UJupiterSettingsSubsystem::GetNotificationDuration() const
{
	return CurrentSettings ? CurrentSettings->NotificationDuration : 3.0f;
}

void UJupiterSettingsSubsystem::SetCameraSpeed(float NewValue)
{
	if (CurrentSettings && CurrentSettings->CameraSpeed != NewValue)
	{
		CurrentSettings->CameraSpeed = NewValue;
		OnSettingsChanged.Broadcast();
	}
}

float UJupiterSettingsSubsystem::GetCameraSpeed() const
{
	return CurrentSettings ? CurrentSettings->CameraSpeed : 2000.0f;
}

void UJupiterSettingsSubsystem::SetRotateSpeed(float NewValue)
{
	if (CurrentSettings && CurrentSettings->RotateSpeed != NewValue)
	{
		CurrentSettings->RotateSpeed = NewValue;
		OnSettingsChanged.Broadcast();
	}
}

float UJupiterSettingsSubsystem::GetRotateSpeed() const
{
	return CurrentSettings ? CurrentSettings->RotateSpeed : 45.0f;
}

void UJupiterSettingsSubsystem::SetEdgeScrollSpeed(float NewValue)
{
	if (CurrentSettings && CurrentSettings->EdgeScrollSpeed != NewValue)
	{
		CurrentSettings->EdgeScrollSpeed = NewValue;
		OnSettingsChanged.Broadcast();
	}
}

float UJupiterSettingsSubsystem::GetEdgeScrollSpeed() const
{
	return CurrentSettings ? CurrentSettings->EdgeScrollSpeed : 50.0f;
}

void UJupiterSettingsSubsystem::SetCanEdgeScroll(bool bNewValue)
{
	if (CurrentSettings && CurrentSettings->bCanEdgeScroll != bNewValue)
	{
		CurrentSettings->bCanEdgeScroll = bNewValue;
		OnSettingsChanged.Broadcast();
	}
}

bool UJupiterSettingsSubsystem::GetCanEdgeScroll() const
{
	return CurrentSettings ? CurrentSettings->bCanEdgeScroll : true;
}

void UJupiterSettingsSubsystem::SetZoomLimits(float NewMin, float NewMax)
{
	if (CurrentSettings)
	{
		bool bChanged = false;
		if (CurrentSettings->MinZoom != NewMin)
		{
			CurrentSettings->MinZoom = NewMin;
			bChanged = true;
		}
		if (CurrentSettings->MaxZoom != NewMax)
		{
			CurrentSettings->MaxZoom = NewMax;
			bChanged = true;
		}

		if (bChanged)
		{
			OnSettingsChanged.Broadcast();
		}
	}
}

void UJupiterSettingsSubsystem::GetZoomLimits(float& OutMin, float& OutMax) const
{
	if (CurrentSettings)
	{
		OutMin = CurrentSettings->MinZoom;
		OutMax = CurrentSettings->MaxZoom;
	}
	else
	{
		OutMin = 300.0f;
		OutMax = 2000.0f;
	}
}
