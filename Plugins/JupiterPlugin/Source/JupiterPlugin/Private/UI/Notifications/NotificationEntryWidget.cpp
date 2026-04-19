#include "UI/Notification/NotificationEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "UI/CustomButtonWidget.h"
#include "TimerManager.h"

void UNotificationEntryWidget::SetupNotification_Implementation(const FNotificationData& Data)
{
    CurrentData = Data;

    if (Text_Title) 
        Text_Title->SetText(Data.Title);
    
    if (Text_SubTitle) 
        Text_SubTitle->SetText(Data.SubTitle);
    
    if (Image_AccentStrip)
    {
        Image_AccentStrip->SetColorAndOpacity(Data.AccentColor);
    }

    if (Image_Icon)
    {
        if (Data.Icon)
        {
            Image_Icon->SetBrushFromTexture(Data.Icon);
            Image_Icon->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    
    if (Btn_Action)
    {
        Btn_Action->OnButtonClicked.RemoveDynamic(this, &UNotificationEntryWidget::OnActionClicked);
        if (Data.Action.IsBound() || Data.NativeAction.IsBound())
        {
             Btn_Action->SetVisibility(ESlateVisibility::Visible);
             Btn_Action->OnButtonClicked.AddDynamic(this, &UNotificationEntryWidget::OnActionClicked);
        }
        else
        {
             Btn_Action->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    PlayIntroAnimation();
    StartDismissTimer(Data.Duration);
}

void UNotificationEntryWidget::ResetState()
{
    GetWorld()->GetTimerManager().ClearTimer(DismissTimerHandle);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UNotificationEntryWidget::OnActionClicked(UCustomButtonWidget* Button, int Index)
{

    UE_LOG(LogTemp, Warning, TEXT("UNotificationEntryWidget::OnActionClicked - Clicked!"));

    if (CurrentData.Action.IsBound())
    {
        UE_LOG(LogTemp, Warning, TEXT("UNotificationEntryWidget::OnActionClicked - Executing BP Action"));
        CurrentData.Action.Execute();
    }
    
    if (CurrentData.NativeAction.IsBound())
    {
        UE_LOG(LogTemp, Warning, TEXT("UNotificationEntryWidget::OnActionClicked - Executing Native Action"));
        CurrentData.NativeAction.Execute();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UNotificationEntryWidget::OnActionClicked - No Native Action Bound"));
    }
}

void UNotificationEntryWidget::StartDismissTimer(float Duration)
{
    if (Duration > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(DismissTimerHandle, this, &UNotificationEntryWidget::HandleDismissTimer, Duration, false);
    }
}

void UNotificationEntryWidget::HandleDismissTimer()
{
    PlayOutroAnimation();
}

void UNotificationEntryWidget::NotifyAnimationFinished()
{
    OnFinished.ExecuteIfBound();
}
