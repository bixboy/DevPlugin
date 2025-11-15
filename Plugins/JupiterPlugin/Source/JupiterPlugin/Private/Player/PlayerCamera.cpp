#include "Player/PlayerCamera.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/CameraCommandSystem.h"
#include "Player/CameraMovementSystem.h"
#include "Player/CameraPatrolSystem.h"
#include "Player/CameraPreviewSystem.h"
#include "Player/CameraSelectionSystem.h"
#include "Player/CameraSpawnSystem.h"

APlayerCamera::APlayerCamera()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
    RootComponent = SceneComponent;

    PawnMovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("PawnMovementComponent"));

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->bDoCollisionTest = false;

    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(SpringArm);

    SelectionComponent = CreateDefaultSubobject<UUnitSelectionComponent>(TEXT("SelectionComponent"));
    OrderComponent = CreateDefaultSubobject<UUnitOrderComponent>(TEXT("OrderComponent"));
    FormationComponent = CreateDefaultSubobject<UUnitFormationComponent>(TEXT("FormationComponent"));
    SpawnComponent = CreateDefaultSubobject<UUnitSpawnComponent>(TEXT("SpawnComponent"));
    PatrolComponent = CreateDefaultSubobject<UUnitPatrolComponent>(TEXT("PatrolComponent"));

    MovementSystemClass = UCameraMovementSystem::StaticClass();
    SelectionSystemClass = UCameraSelectionSystem::StaticClass();
    CommandSystemClass = UCameraCommandSystem::StaticClass();
    PatrolSystemClass = UCameraPatrolSystem::StaticClass();
    SpawnSystemClass = UCameraSpawnSystem::StaticClass();
    PreviewSystemClass = UCameraPreviewSystem::StaticClass();


void APlayerCamera::BeginPlay()
{
    Super::BeginPlay();
    CachePlayerController();
    if (Player.IsValid())
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        Player->SetInputMode(InputMode);
        Player->bShowMouseCursor = true;
    }
    InitializeSystems();


void APlayerCamera::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    const auto TickSystem = [DeltaTime](auto* System)
    {
        if (System)
        {
            System->Tick(DeltaTime);
        }
    };
    TickSystem(MovementSystem);
    TickSystem(SelectionSystem);
    TickSystem(CommandSystem);
    TickSystem(PatrolSystem);
    TickSystem(SpawnSystem);
    TickSystem(PreviewSystem);


void APlayerCamera::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(Input))
    {
        BindDefaultInputs(EnhancedInput);
    }


void APlayerCamera::NotifyControllerChanged()
{
    if (const APlayerController* PreviousPlayer = Cast<APlayerController>(PreviousController))
    {
        if (ULocalPlayer* LocalPlayer = PreviousPlayer->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
            {
                InputSubsystem->RemoveMappingContext(InputMappingContext);
            }
        }
    }
    CachePlayerController();
    if (Player.IsValid())
    {
        Player->InputYawScale_DEPRECATED = Player->InputPitchScale_DEPRECATED = Player->InputRollScale_DEPRECATED = 1.f;

        if (ULocalPlayer* LocalPlayer = Player->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
            {
                FModifyContextOptions Options;
                Options.bNotifyUserSettings = true;
                InputSubsystem->AddMappingContext(InputMappingContext, 0, Options);
            }
        }
    }
    Super::NotifyControllerChanged();


void APlayerCamera::UnPossessed()
{
    Super::UnPossessed();
    if (APlayerController* CurrentController = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LocalPlayer = CurrentController->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
            {
                InputSubsystem->RemoveMappingContext(InputMappingContext);
            }
        }
    }


void APlayerCamera::InitializeSystems()
{
    PreviewSystem = CreateSystem(PreviewSystemClass);
    if (PreviewSystem)
    {
        PreviewSystem->Init(this);
    }

    MovementSystem = CreateSystem(MovementSystemClass);
    if (MovementSystem)
    {
        MovementSystem->Init(this);
    }

    CommandSystem = CreateSystem(CommandSystemClass);
    if (CommandSystem)
    {
        CommandSystem->Init(this);
        if (PreviewSystem)
        {
            CommandSystem->SetPreviewSystem(PreviewSystem);
        }
    }

    SpawnSystem = CreateSystem(SpawnSystemClass);
    if (SpawnSystem)
    {
        SpawnSystem->Init(this);
        if (PreviewSystem)
        {
            SpawnSystem->SetPreviewSystem(PreviewSystem);
        }
        if (CommandSystem)
        {
            SpawnSystem->SetCommandSystem(CommandSystem);
        }
    }

    PatrolSystem = CreateSystem(PatrolSystemClass);
    if (PatrolSystem)
    {
        PatrolSystem->Init(this);
        if (CommandSystem)
        {
            PatrolSystem->SetCommandSystem(CommandSystem);
            CommandSystem->SetPatrolSystem(PatrolSystem);
        }
    }

    SelectionSystem = CreateSystem(SelectionSystemClass);
    if (SelectionSystem)
    {
        SelectionSystem->Init(this);
        if (CommandSystem)
        {
            SelectionSystem->SetCommandSystem(CommandSystem);
        }
        if (SpawnSystem)
        {
            SelectionSystem->SetSpawnSystem(SpawnSystem);
        }
        if (PatrolSystem)
        {
            SelectionSystem->SetPatrolSystem(PatrolSystem);
            PatrolSystem->SetSelectionSystem(SelectionSystem);
        }
    }

    if (CommandSystem && SpawnSystem)
    {
        CommandSystem->SetSpawnSystem(SpawnSystem);
    }


void APlayerCamera::BindDefaultInputs(UEnhancedInputComponent* EnhancedInput)
{
    if (!EnhancedInput)
    {
        return;
    }
    EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCamera::OnMove);
    EnhancedInput->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &APlayerCamera::OnZoom);
    EnhancedInput->BindAction(EnableRotateAction, ETriggerEvent::Triggered, this, &APlayerCamera::OnEnableRotate);
    EnhancedInput->BindAction(RotateHorizontalAction, ETriggerEvent::Triggered, this, &APlayerCamera::OnRotateHorizontal);
    EnhancedInput->BindAction(RotateVerticalAction, ETriggerEvent::Triggered, this, &APlayerCamera::OnRotateVertical);
    EnhancedInput->BindAction(SelectAction, ETriggerEvent::Started, this, &APlayerCamera::OnSelectionPressed);
    EnhancedInput->BindAction(SelectAction, ETriggerEvent::Completed, this, &APlayerCamera::OnSelectionReleased);
    EnhancedInput->BindAction(SelectHoldAction, ETriggerEvent::Triggered, this, &APlayerCamera::OnSelectionHold);
    EnhancedInput->BindAction(DoubleTapAction, ETriggerEvent::Completed, this, &APlayerCamera::OnSelectAll);
    EnhancedInput->BindAction(CommandAction, ETriggerEvent::Started, this, &APlayerCamera::OnCommandActionStarted);
    EnhancedInput->BindAction(CommandAction, ETriggerEvent::Completed, this, &APlayerCamera::OnCommandActionCompleted);
    EnhancedInput->BindAction(AltCommandAction, ETriggerEvent::Started, this, &APlayerCamera::OnAltPressed);
    EnhancedInput->BindAction(AltCommandAction, ETriggerEvent::Completed, this, &APlayerCamera::OnAltReleased);
    EnhancedInput->BindAction(AltCommandHoldAction, ETriggerEvent::Triggered, this, &APlayerCamera::OnAltHold);
    EnhancedInput->BindAction(PatrolCommandAction, ETriggerEvent::Triggered, this, &APlayerCamera::OnPatrolTriggered);
    EnhancedInput->BindAction(DeleteCommandAction, ETriggerEvent::Triggered, this, &APlayerCamera::OnDestroySelected);
    EnhancedInput->BindAction(SpawnUnitAction, ETriggerEvent::Completed, this, &APlayerCamera::OnSpawnTriggered);


void APlayerCamera::OnMove(const FInputActionValue& Value)
{
    if (MovementSystem)
    {
        MovementSystem->HandleMoveInput(Value);
    }
}

void APlayerCamera::OnZoom(const FInputActionValue& Value)
{
    if (MovementSystem)
    {
        MovementSystem->HandleZoomInput(Value);
    }
}

void APlayerCamera::OnRotateHorizontal(const FInputActionValue& Value)
{
    if (MovementSystem)
    {
        MovementSystem->HandleRotateHorizontal(Value);
    }
}

void APlayerCamera::OnRotateVertical(const FInputActionValue& Value)
{
    if (MovementSystem)
    {
        MovementSystem->HandleRotateVertical(Value);
    }
}

void APlayerCamera::OnEnableRotate(const FInputActionValue& Value)
{
    if (MovementSystem)
    {
        MovementSystem->HandleRotateToggle(Value);
    }
}

void APlayerCamera::OnSelectionPressed()
{
    if (SelectionSystem)
    {
        SelectionSystem->HandleSelectionPressed();
    }
}

void APlayerCamera::OnSelectionReleased()
{
    if (SelectionSystem)
    {
        SelectionSystem->HandleSelectionReleased();
    }
}

void APlayerCamera::OnSelectionHold(const FInputActionValue& Value)
{
    if (SelectionSystem)
    {
        SelectionSystem->HandleSelectionHold(Value);
    }
}

void APlayerCamera::OnSelectAll()
{
    if (SelectionSystem)
    {
        SelectionSystem->HandleSelectAll();
    }
}

void APlayerCamera::OnCommandActionStarted()
{
    if (CommandSystem)
    {
        CommandSystem->HandleCommandActionStarted();
    }
}

void APlayerCamera::OnCommandActionCompleted()
{
    if (CommandSystem)
    {
        CommandSystem->HandleCommandActionCompleted();
    }
}

void APlayerCamera::OnAltPressed()
{
    if (PatrolSystem)
    {
        PatrolSystem->HandleAltPressed();
    }
}

void APlayerCamera::OnAltReleased()
{
    if (PatrolSystem)
    {
        PatrolSystem->HandleAltReleased();
    }
}

void APlayerCamera::OnAltHold(const FInputActionValue& Value)
{
    if (PatrolSystem)
    {
        PatrolSystem->HandleAltHold(Value);
    }
}

void APlayerCamera::OnPatrolTriggered(const FInputActionValue& Value)
{
    if (PatrolSystem)
    {
        PatrolSystem->HandlePatrolRadius(Value);
    }
}

void APlayerCamera::OnDestroySelected()
{
    if (CommandSystem)
    {
        CommandSystem->HandleDestroySelected();
    }
}

void APlayerCamera::OnSpawnTriggered()
{
    if (SpawnSystem)
    {
        SpawnSystem->HandleSpawnInput();
    }
}


void APlayerCamera::Server_DestroyActor_Implementation(const TArray<AActor*>& ActorToDestroy)
{
    if (CommandSystem)
    {
        CommandSystem->HandleServerDestroyActor(ActorToDestroy);
    }
}

void APlayerCamera::CachePlayerController()
{
    Player = Cast<APlayerController>(GetInstigatorController());
    if (!Player.IsValid())
    {
        Player = Cast<APlayerController>(GetController());
    }
}

