#include "UI/Editor/Pages/Page_Settings.h"

#include "UI/CustomSliderWidget.h"
#include "UI/JupiterToggleSwitch.h"
#include "UI/Notification/NotificationSubsystem.h"

#include "Subsystems/JupiterSettingsSubsystem.h"

#include "Components/EditableTextBox.h"



UPage_Settings::UPage_Settings()
{
    PageTitle = FText::FromString(TEXT("Settings"));
    bIsFooterPage = true;
}

void UPage_Settings::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (Slider_TooltipDelay)
        Slider_TooltipDelay->OnValueChanged.AddDynamic(this, &UPage_Settings::OnTooltipDelayChanged);

    if (Slider_NotifDuration)
        Slider_NotifDuration->OnValueChanged.AddDynamic(this, &UPage_Settings::OnNotifDurationChanged);

    if (Slider_CamSpeed)
        Slider_CamSpeed->OnValueChanged.AddDynamic(this, &UPage_Settings::OnCamSpeedChanged);

    if (Slider_RotSpeed)
        Slider_RotSpeed->OnValueChanged.AddDynamic(this, &UPage_Settings::OnRotSpeedChanged);

    if (Slider_EdgeScrollSpeed)
        Slider_EdgeScrollSpeed->OnValueChanged.AddDynamic(this, &UPage_Settings::OnEdgeScrollSpeedChanged);

    if (CheckBox_EdgeScroll)
        CheckBox_EdgeScroll->OnToggled.AddDynamic(this, &UPage_Settings::OnEdgeScrollCheckChanged);

    if (Slider_MinZoom)
        Slider_MinZoom->OnValueChanged.AddDynamic(this, &UPage_Settings::OnMinZoomChanged);

    if (Slider_MaxZoom)
        Slider_MaxZoom->OnValueChanged.AddDynamic(this, &UPage_Settings::OnMaxZoomChanged);

    // --- Input Bindings ---
    if (Input_TooltipDelay)
        Input_TooltipDelay->OnTextCommitted.AddDynamic(this, &UPage_Settings::OnTooltipDelayTextCommitted);

    if (Input_NotifDuration)
        Input_NotifDuration->OnTextCommitted.AddDynamic(this, &UPage_Settings::OnNotifDurationTextCommitted);

    if (Input_CamSpeed)
        Input_CamSpeed->OnTextCommitted.AddDynamic(this, &UPage_Settings::OnCamSpeedTextCommitted);
        
    if (Input_RotSpeed)
        Input_RotSpeed->OnTextCommitted.AddDynamic(this, &UPage_Settings::OnRotSpeedTextCommitted);
        
    if (Input_EdgeScrollSpeed)
        Input_EdgeScrollSpeed->OnTextCommitted.AddDynamic(this, &UPage_Settings::OnEdgeScrollSpeedTextCommitted);

    if (Input_MinZoom)
        Input_MinZoom->OnTextCommitted.AddDynamic(this, &UPage_Settings::OnMinZoomTextCommitted);

    if (Input_MaxZoom)
        Input_MaxZoom->OnTextCommitted.AddDynamic(this, &UPage_Settings::OnMaxZoomTextCommitted);
}

void UPage_Settings::OnPageOpened()
{
    Super::OnPageOpened();
    
    bInitializing = true;

    if (Slider_TooltipDelay)
    {
        Slider_TooltipDelay->SetMinMax(0.0f, 5.0f);
        Slider_TooltipDelay->SetValue(GetTooltipDelay());
    }
    
    if (Slider_NotifDuration)
    {
        Slider_NotifDuration->SetMinMax(1.0f, 20.0f);
        Slider_NotifDuration->SetValue(GetNotificationDuration());
    }
    
    if (Slider_CamSpeed)
    {
        Slider_CamSpeed->SetMinMax(0.0f, 5000.0f);
        Slider_CamSpeed->SetValue(GetCameraSpeed());
    }

    if (Slider_RotSpeed)
    {
        Slider_RotSpeed->SetMinMax(0.1f, 100.0f);
        Slider_RotSpeed->SetValue(GetRotateSpeed());
    }

    if (Slider_EdgeScrollSpeed)
    {
        Slider_EdgeScrollSpeed->SetMinMax(0.0f, 200.0f);
        Slider_EdgeScrollSpeed->SetValue(GetEdgeScrollSpeed());
    }

    if (CheckBox_EdgeScroll)
    {
        CheckBox_EdgeScroll->SetIsToggled(GetCanEdgeScroll());
    }
    
    float MinZ, MaxZ;
    GetZoomLimits(MinZ, MaxZ);

    if (Slider_MinZoom)
    {
        Slider_MinZoom->SetMinMax(100.0f, 10000.0f);
        Slider_MinZoom->SetValue(MinZ);
    }
    
    if (Slider_MaxZoom)
    {
        Slider_MaxZoom->SetMinMax(100.0f, 10000.0f);
        Slider_MaxZoom->SetValue(MaxZ);
    }

    // --- Sync Text Boxes ---
    if (Input_TooltipDelay)
        Input_TooltipDelay->SetText(FText::AsNumber(GetTooltipDelay()));

    if (Input_NotifDuration)
        Input_NotifDuration->SetText(FText::AsNumber(GetNotificationDuration()));

    if (Input_CamSpeed)
        Input_CamSpeed->SetText(FText::AsNumber(GetCameraSpeed()));

    if (Input_RotSpeed)
        Input_RotSpeed->SetText(FText::AsNumber(GetRotateSpeed()));

    if (Input_EdgeScrollSpeed)
        Input_EdgeScrollSpeed->SetText(FText::AsNumber(GetEdgeScrollSpeed()));

    if (Input_MinZoom)
        Input_MinZoom->SetText(FText::AsNumber(MinZ));

    if (Input_MaxZoom)
        Input_MaxZoom->SetText(FText::AsNumber(MaxZ));
    
    bInitializing = false;
}

// --- Handlers ---

void UPage_Settings::OnTooltipDelayChanged(float Value)
{
    if (bInitializing) 
    	return;
	
    SetTooltipDelay(Value);
	
    if (Input_TooltipDelay)
    	Input_TooltipDelay->SetText(FText::AsNumber(Value));
}

void UPage_Settings::OnTooltipDelayTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (bInitializing) 
        return;

    if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
    {
        if (Text.IsNumeric())
        {
            float Val = FCString::Atof(*Text.ToString());
            SetTooltipDelay(Val);
            if (Slider_TooltipDelay) 
                Slider_TooltipDelay->SetValue(Val);
        }

        if (Input_TooltipDelay) 
            Input_TooltipDelay->SetText(FText::AsNumber(GetTooltipDelay()));
    }
}

void UPage_Settings::OnNotifDurationChanged(float Value)
{
    if (bInitializing) 
        return;

    SetNotificationDuration(Value);

    if (Input_NotifDuration) 
        Input_NotifDuration->SetText(FText::AsNumber(Value));
}

void UPage_Settings::OnNotifDurationTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (bInitializing) 
        return;

    if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
    {
        if (Text.IsNumeric())
        {
            float Val = FCString::Atof(*Text.ToString());
            SetNotificationDuration(Val);

            if (Slider_NotifDuration) 
                Slider_NotifDuration->SetValue(Val);
        }

        if (Input_NotifDuration) 
            Input_NotifDuration->SetText(FText::AsNumber(GetNotificationDuration()));
    }
}

void UPage_Settings::OnCamSpeedChanged(float Value)
{
    if (bInitializing) 
        return;

    SetCameraSpeed(Value);

    if (Input_CamSpeed) 
        Input_CamSpeed->SetText(FText::AsNumber(Value));
}

void UPage_Settings::OnCamSpeedTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (bInitializing) 
        return;

    if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
    {
        if (Text.IsNumeric())
        {
            float Val = FCString::Atof(*Text.ToString());
            SetCameraSpeed(Val);

            if (Slider_CamSpeed) 
                Slider_CamSpeed->SetValue(Val);
        }
        if (Input_CamSpeed) Input_CamSpeed->SetText(FText::AsNumber(GetCameraSpeed()));
    }
}

void UPage_Settings::OnRotSpeedChanged(float Value)
{
    if (bInitializing) 
        return;

    SetRotateSpeed(Value);

    if (Input_RotSpeed) 
        Input_RotSpeed->SetText(FText::AsNumber(Value));
}

void UPage_Settings::OnRotSpeedTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (bInitializing) 
        return;
    if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
    {
        if (Text.IsNumeric())
        {
            float Val = FCString::Atof(*Text.ToString());
            SetRotateSpeed(Val);
            if (Slider_RotSpeed) Slider_RotSpeed->SetValue(Val);
        }
        if (Input_RotSpeed) Input_RotSpeed->SetText(FText::AsNumber(GetRotateSpeed()));
    }
}

void UPage_Settings::OnEdgeScrollSpeedChanged(float Value)
{
    if (bInitializing) return;
    SetEdgeScrollSpeed(Value);
    if (Input_EdgeScrollSpeed) Input_EdgeScrollSpeed->SetText(FText::AsNumber(Value));
}

void UPage_Settings::OnEdgeScrollSpeedTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (bInitializing) return;
    if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
    {
        if (Text.IsNumeric())
        {
            float Val = FCString::Atof(*Text.ToString());
            SetEdgeScrollSpeed(Val);
            if (Slider_EdgeScrollSpeed) Slider_EdgeScrollSpeed->SetValue(Val);
        }
        if (Input_EdgeScrollSpeed) Input_EdgeScrollSpeed->SetText(FText::AsNumber(GetEdgeScrollSpeed()));
    }
}

void UPage_Settings::OnEdgeScrollCheckChanged(bool bIsChecked)
{
    if (bInitializing) return;
    SetCanEdgeScroll(bIsChecked);
}

void UPage_Settings::OnMinZoomChanged(float Value)
{
    if (bInitializing) return;
    float Min, Max;
    GetZoomLimits(Min, Max);
    SetZoomLimits(Value, Max);
    if (Input_MinZoom) Input_MinZoom->SetText(FText::AsNumber(Value));
}

void UPage_Settings::OnMinZoomTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (bInitializing) return;
    if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
    {
        float Min, Max;
        GetZoomLimits(Min, Max);
        
        if (Text.IsNumeric())
        {
            float Val = FCString::Atof(*Text.ToString());
            SetZoomLimits(Val, Max);
            if (Slider_MinZoom) Slider_MinZoom->SetValue(Val);
        }
        
        // Refresh with actual value (in case it was clamped internally)
        GetZoomLimits(Min, Max);
        if (Input_MinZoom) Input_MinZoom->SetText(FText::AsNumber(Min));
    }
}

void UPage_Settings::OnMaxZoomChanged(float Value)
{
    if (bInitializing) return;
    float Min, Max;
    GetZoomLimits(Min, Max);
    SetZoomLimits(Min, Value);
    if (Input_MaxZoom) Input_MaxZoom->SetText(FText::AsNumber(Value));
}

void UPage_Settings::OnMaxZoomTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (bInitializing) return;
    if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
    {
        float Min, Max;
        GetZoomLimits(Min, Max);

        if (Text.IsNumeric())
        {
            float Val = FCString::Atof(*Text.ToString());
            SetZoomLimits(Min, Val);
            if (Slider_MaxZoom) Slider_MaxZoom->SetValue(Val);
        }

        // Refresh with actual value
        GetZoomLimits(Min, Max);
        if (Input_MaxZoom) Input_MaxZoom->SetText(FText::AsNumber(Max));
    }
}

// --- Tooltips ---

void UPage_Settings::SetTooltipDelay(float Val)
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        Settings->SetTooltipDelay(Val);
        Settings->SaveSettings();
    }
}

float UPage_Settings::GetTooltipDelay() const
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        return Settings->GetTooltipDelay();
    }
    return 0.5f;
}

// --- Notifications ---

void UPage_Settings::SetNotificationDuration(float Val)
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        Settings->SetNotificationDuration(Val);
        Settings->SaveSettings();
    }
}

float UPage_Settings::GetNotificationDuration() const
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        return Settings->GetNotificationDuration();
    }
    return 3.0f;
}

// --- Camera & Controls ---

void UPage_Settings::SetCameraSpeed(float Val)
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        Settings->SetCameraSpeed(Val);
        Settings->SaveSettings();
    }
}

float UPage_Settings::GetCameraSpeed() const
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        return Settings->GetCameraSpeed();
    }
    return 2000.0f;
}

void UPage_Settings::SetRotateSpeed(float Val)
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        Settings->SetRotateSpeed(Val);
        Settings->SaveSettings();
    }
}

float UPage_Settings::GetRotateSpeed() const
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        return Settings->GetRotateSpeed();
    }
    return 45.0f;
}

void UPage_Settings::SetEdgeScrollSpeed(float Val)
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        Settings->SetEdgeScrollSpeed(Val);
        Settings->SaveSettings();
    }
}

float UPage_Settings::GetEdgeScrollSpeed() const
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        return Settings->GetEdgeScrollSpeed();
    }
    return 50.0f;
}

void UPage_Settings::SetCanEdgeScroll(bool bEnable)
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        Settings->SetCanEdgeScroll(bEnable);
        Settings->SaveSettings();
    }
}

bool UPage_Settings::GetCanEdgeScroll() const
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        return Settings->GetCanEdgeScroll();
    }
    return true;
}

void UPage_Settings::SetZoomLimits(float NewMin, float NewMax)
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        Settings->SetZoomLimits(NewMin, NewMax);
        Settings->SaveSettings();
    }
}

void UPage_Settings::GetZoomLimits(float& OutMin, float& OutMax) const
{
    if (UJupiterSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UJupiterSettingsSubsystem>())
    {
        Settings->GetZoomLimits(OutMin, OutMax);
    }
    else
    {
        OutMin = 300.0f;
        OutMax = 2000.0f;
    }
}


