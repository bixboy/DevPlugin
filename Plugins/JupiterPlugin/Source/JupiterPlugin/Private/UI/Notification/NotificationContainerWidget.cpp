#include "UI/Notification/NotificationContainerWidget.h"
#include "UI/Notification/NotificationEntryWidget.h"
#include "UI/Notification/NotificationSubsystem.h"
#include "Components/VerticalBox.h"


void UNotificationContainerWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UNotificationSubsystem* System = GI->GetSubsystem<UNotificationSubsystem>())
        {
            System->OnNotificationAdded.AddDynamic(this, &UNotificationContainerWidget::OnNotificationAdded);
            System->OnClearAll.AddDynamic(this, &UNotificationContainerWidget::OnClearAll);
        }
    }
	
	if (NotificationList)
		NotificationList->ClearChildren();
}

void UNotificationContainerWidget::NativeDestruct()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UNotificationSubsystem* System = GI->GetSubsystem<UNotificationSubsystem>())
        {
            System->OnNotificationAdded.RemoveAll(this);
            System->OnClearAll.RemoveAll(this);
        }
    }
    
    Super::NativeDestruct();
}

void UNotificationContainerWidget::OnNotificationAdded(const FNotificationData& Data)
{
    if (!NotificationList || !NotificationClass) 
        return;

    if (ActiveWidgets.Num() >= MaxNotifications)
    {
        UNotificationEntryWidget* Oldest = ActiveWidgets[0];
        RecycleWidget(Oldest);
    }

    TSubclassOf<UNotificationEntryWidget> ClassToUse = NotificationClass;
    if (Data.VisualClass && Data.VisualClass->IsChildOf(UNotificationEntryWidget::StaticClass()))
    {
        ClassToUse = *Data.VisualClass;
    }

    UNotificationEntryWidget* NewWidget = GetWidgetFromPool(ClassToUse);
    if (NewWidget)
    {
        NewWidget->SetupNotification(Data);
        NewWidget->SetVisibility(ESlateVisibility::Visible);
        NotificationList->AddChild(NewWidget);
        ActiveWidgets.Add(NewWidget);
        
        NewWidget->OnFinished.BindWeakLambda(this, [this, NewWidget]()
        {
             RecycleWidget(NewWidget);
        });
    }
}

void UNotificationContainerWidget::OnClearAll(const FNotificationData& Data)
{
    while (ActiveWidgets.Num() > 0)
    {
        RecycleWidget(ActiveWidgets[0]);
    }
}

void UNotificationContainerWidget::RecycleWidget(UNotificationEntryWidget* Widget)
{
    if (!Widget) 
        return;

    if (NotificationList)
    {
        NotificationList->RemoveChild(Widget);
    }
    
    ActiveWidgets.Remove(Widget);
    
    Widget->ResetState();
    Widget->OnFinished.Unbind();
    
    // Add to pool for this class
    UClass* WidgetClass = Widget->GetClass();
    FNotificationWidgetPool& Pool = InactivePools.FindOrAdd(WidgetClass);
    Pool.Widgets.Add(Widget);
}

UNotificationEntryWidget* UNotificationContainerWidget::GetWidgetFromPool(TSubclassOf<UNotificationEntryWidget> RequestedClass)
{
    if (!RequestedClass) return nullptr;

    UClass* ClassPtr = RequestedClass.Get();
    FNotificationWidgetPool* Pool = InactivePools.Find(ClassPtr);

    if (Pool && Pool->Widgets.Num() > 0)
    {
         return Pool->Widgets.Pop();
    }

    // Create New
    return CreateWidget<UNotificationEntryWidget>(this, RequestedClass);
}
