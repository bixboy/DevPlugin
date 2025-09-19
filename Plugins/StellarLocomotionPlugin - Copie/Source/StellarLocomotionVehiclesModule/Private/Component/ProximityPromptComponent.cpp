#include "Component/ProximityPromptComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Component/ProximityPromptManagerComponent.h"
#include "Widgets/PromptWidget.h"


UProximityPromptComponent::UProximityPromptComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    
    PromptWidgetComponent = nullptr;
}

void UProximityPromptComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner()->GetNetMode() == NM_DedicatedServer)
        return;

    GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UProximityPromptComponent::TryRegisterPrompt);

    if (PromptWidgetClass)
    {
        PromptWidgetComponent = NewObject<UWidgetComponent>(GetOwner());
        PromptWidgetComponent->RegisterComponent();
        PromptWidgetComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);

        PromptWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
        PromptWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PromptWidgetComponent->SetDrawSize(FVector2D(200.f, 100.f));
        PromptWidgetComponent->SetWidgetClass(PromptWidgetClass);
        PromptWidgetComponent->InitWidget();
        PromptWidgetComponent->SetWorldScale3D(FVector(WidgetScale));
        PromptWidgetComponent->SetHiddenInGame(true);

        PromptWidget = Cast<UPromptWidget>(PromptWidgetComponent->GetUserWidgetObject());
        UpdatePromptText();
    }
}

void UProximityPromptComponent::TryRegisterPrompt()
{
    LocalPC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!LocalPC)
        return;

    if (APawn* Pawn = LocalPC->GetPawn())
    {
        if (auto* Manager = Pawn->FindComponentByClass<UProximityPromptManagerComponent>())
        {
            Manager->RegisterPrompt(this);
        }
    }
}

void UProximityPromptComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (PromptWidgetComponent)
        PromptWidgetComponent->DestroyComponent();

    Super::EndPlay(EndPlayReason);
}

void UProximityPromptComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsVisible && LocalPC && LocalPC->IsLocalController())
    {
        FVector CamLoc = LocalPC->PlayerCameraManager->GetCameraLocation();
        FVector Dir    = (CamLoc - PromptWidgetComponent->GetComponentLocation()).GetSafeNormal();
        FRotator R     = Dir.Rotation();
        R.Pitch = R.Roll = 0.f;
        
        PromptWidgetComponent->SetWorldRotation(R);
    }
}


// ======== Visible/Enabled ========
void UProximityPromptComponent::SetPromptVisible(bool bNewVisible)
{
    if (bIsVisible == bNewVisible)
        return;

    bIsVisible = bNewVisible;
    
    if (PromptWidgetComponent)
    {
        PromptWidgetComponent->SetHiddenInGame(!bNewVisible);
        PromptWidgetComponent->RecreateRenderState_Concurrent();
    }

    if (bNewVisible)
        UpdatePromptText();
}

void UProximityPromptComponent::SetPromptEnabled(bool bNewEnabled)
{
    bIsEnabled = bNewEnabled;

    if (!bNewEnabled)
        SetPromptVisible(false);
}



// ======== Text ========
void UProximityPromptComponent::UpdatePromptText()
{
    if (!PromptWidget || !InteractInputAction)
        return;

    TArray<FKey> Keys = GetKeysForAction(InteractInputAction);
    if (Keys.IsEmpty())
        return;

    TArray<FText> Names;
    Names.Reserve(Keys.Num());
    for (const FKey& K : Keys)
        Names.Add(K.GetDisplayName());

    FText Joined = FText::Join(FText::FromString(TEXT(" / ")), Names);
    
    PromptWidget->SetPromptText(
        FText::Format(
            NSLOCTEXT("ProximityPrompt","Fmt","Appuyez sur {0} pour interagir"),
            Joined
        )
    );
}

TArray<FKey> UProximityPromptComponent::GetKeysForAction(UInputAction* Action) const
{
    TArray<FKey> Result;
    if (!Action || !LocalPC)
        return Result;

    if (auto* Sub = LocalPC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        Result = Sub->QueryKeysMappedToAction(Action);

    return Result;
}