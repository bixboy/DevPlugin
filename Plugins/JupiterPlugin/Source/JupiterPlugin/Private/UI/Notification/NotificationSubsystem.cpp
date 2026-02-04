#include "UI/Notification/NotificationSubsystem.h"


void UNotificationSubsystem::AddNotification(const FNotificationData& Data)
{
    OnNotificationAdded.Broadcast(Data);
}

void UNotificationSubsystem::ClearAll()
{
    FNotificationData Dummy;
    OnClearAll.Broadcast(Dummy);
}

void UNotificationSubsystem::NativeNotify(const FText& Title, const FText& Message, FLinearColor Color, float Duration)
{
    FNotificationData Data;
    Data.Title = Title;
    Data.SubTitle = Message;
    Data.AccentColor = Color;
    Data.Duration = Duration;
    
    AddNotification(Data);
}
