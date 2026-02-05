#pragma once
#include "CoreMinimal.h"
#include "UI/Editor/JupiterPageBase.h"
#include "Page_Settings.generated.h"

class UCustomSliderWidget;
class UJupiterToggleSwitch;
class UEditableTextBox;


UCLASS()
class JUPITERPLUGIN_API UPage_Settings : public UJupiterPageBase
{
	GENERATED_BODY()
	
public:
    UPage_Settings();

    virtual void NativeOnInitialized() override;
    virtual void OnPageOpened() override;

protected:
    bool bInitializing = false;

    // --- UI Widgets ---
    UPROPERTY(meta = (BindWidgetOptional))
    UCustomSliderWidget* Slider_TooltipDelay;

    UPROPERTY(meta = (BindWidgetOptional))
	UCustomSliderWidget* Slider_NotifDuration;

    UPROPERTY(meta = (BindWidgetOptional))
    UCustomSliderWidget* Slider_CamSpeed;

    UPROPERTY(meta = (BindWidgetOptional))
    UCustomSliderWidget* Slider_RotSpeed;

    UPROPERTY(meta = (BindWidgetOptional))
    UCustomSliderWidget* Slider_EdgeScrollSpeed;

    UPROPERTY(meta = (BindWidgetOptional))
	UJupiterToggleSwitch* CheckBox_EdgeScroll;

    UPROPERTY(meta = (BindWidgetOptional))
    UCustomSliderWidget* Slider_MinZoom;

    UPROPERTY(meta = (BindWidgetOptional))
    UCustomSliderWidget* Slider_MaxZoom;

    // --- Input Boxes ---
    UPROPERTY(meta = (BindWidgetOptional))
    UEditableTextBox* Input_TooltipDelay;
    
    UPROPERTY(meta = (BindWidgetOptional))
    UEditableTextBox* Input_NotifDuration;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableTextBox* Input_CamSpeed;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableTextBox* Input_RotSpeed;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableTextBox* Input_EdgeScrollSpeed;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableTextBox* Input_MinZoom;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableTextBox* Input_MaxZoom;


    // --- Handlers ---
    UFUNCTION()
    void OnTooltipDelayChanged(float Value);
    
    UFUNCTION()
    void OnTooltipDelayTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnNotifDurationChanged(float Value);

    UFUNCTION()
    void OnNotifDurationTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnCamSpeedChanged(float Value);
    
    UFUNCTION()
    void OnCamSpeedTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
    
    UFUNCTION()
    void OnRotSpeedChanged(float Value);
    
    UFUNCTION()
    void OnRotSpeedTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnEdgeScrollSpeedChanged(float Value);

    UFUNCTION()
    void OnEdgeScrollSpeedTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
    
    UFUNCTION()
    void OnEdgeScrollCheckChanged(bool bIsChecked);

    UFUNCTION()
    void OnMinZoomChanged(float Value);

    UFUNCTION()
    void OnMinZoomTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
    
    UFUNCTION()
    void OnMaxZoomChanged(float Value);

    UFUNCTION()
    void OnMaxZoomTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

public:
    // --- Tooltips ---
    UFUNCTION(BlueprintCallable, Category = "Settings|Tooltip")
    void SetTooltipDelay(float Val);

    UFUNCTION(BlueprintCallable, Category = "Settings|Tooltip")
    float GetTooltipDelay() const;

    // --- Notifications ---
    UFUNCTION(BlueprintCallable, Category = "Settings|Notification")
    void SetNotificationDuration(float Val);

    UFUNCTION(BlueprintCallable, Category = "Settings|Notification")
    float GetNotificationDuration() const;

    // --- Camera & Controls ---
    UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
    void SetCameraSpeed(float Val);

    UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
    float GetCameraSpeed() const;

    UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
    void SetRotateSpeed(float Val);

    UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
    float GetRotateSpeed() const;

    UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
    void SetEdgeScrollSpeed(float Val);

    UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
    float GetEdgeScrollSpeed() const;

    UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
    void SetCanEdgeScroll(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
    bool GetCanEdgeScroll() const;

    UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
    void SetZoomLimits(float NewMin, float NewMax);

    UFUNCTION(BlueprintCallable, Category = "Settings|Camera")
    void GetZoomLimits(float& OutMin, float& OutMax) const;
};
