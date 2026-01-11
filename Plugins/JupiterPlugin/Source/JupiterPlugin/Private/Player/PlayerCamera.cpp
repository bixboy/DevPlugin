#include "Player/PlayerCamera.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Camera/CameraComponent.h"
#include "Components/Unit/UnitFormationComponent.h"
#include "Components/Unit/UnitOrderComponent.h"
#include "Components/Patrol/UnitPatrolComponent.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "Components/Unit/UnitSpawnComponent.h"
#include "Player/JupiterPlayerSystem/CameraCommandSystem.h"
#include "Player/JupiterPlayerSystem/CameraMovementSystem.h"
#include "Player/JupiterPlayerSystem/CameraPreviewSystem.h"
#include "Player/JupiterPlayerSystem/CameraSelectionSystem.h"
#include "Player/JupiterPlayerSystem/CameraSpawnSystem.h"


APlayerCamera::APlayerCamera()
{
    PrimaryActorTick.bCanEverTick = true;

    // ------- Build Camera -------
    SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = SceneComponent;

    PawnMovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->bDoCollisionTest = false;

    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    CameraComponent->SetupAttachment(SpringArm);

    // ------- Gameplay Components -------
    SelectionComponent = CreateDefaultSubobject<UUnitSelectionComponent>(TEXT("Selection"));
    OrderComponent = CreateDefaultSubobject<UUnitOrderComponent>(TEXT("Order"));
    FormationComponent = CreateDefaultSubobject<UUnitFormationComponent>(TEXT("Formation"));
    SpawnComponent = CreateDefaultSubobject<UUnitSpawnComponent>(TEXT("Spawn"));
    PatrolComponent = CreateDefaultSubobject<UUnitPatrolComponent>(TEXT("Patrol"));
}

void APlayerCamera::BeginPlay()
{
    Super::BeginPlay();

    Player = Cast<APlayerController>(GetController());
    
    if (Player.IsValid())
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        Player->SetInputMode(Mode);
        Player->bShowMouseCursor = true;

        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player->GetLocalPlayer()))
        {
            if (InputMapping)
            {
                Subsystem->AddMappingContext(InputMapping, 0);
            }
        }
    }

    InitializeSystems();
}

void APlayerCamera::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!Player.IsValid())
    	return;

    if (SpringArm)
    {
        SpringArm->TargetArmLength = TargetZoom; 
        SpringArm->SetRelativeRotation(TargetRotation);
    }

    // Tick Systems
    if (MovementSystem)
    	MovementSystem->Tick(DeltaTime);
	
    if (SelectionSystem)
    	SelectionSystem->Tick(DeltaTime);
	
    if (CommandSystem)
    	CommandSystem->Tick(DeltaTime);
	
    if (SpawnSystem)
    	SpawnSystem->Tick(DeltaTime);
	
    if (PreviewSystem)
    	PreviewSystem->Tick(DeltaTime);
}

void APlayerCamera::SetupPlayerInputComponent(UInputComponent* Input)
{
    InitializeSystems();

    Super::SetupPlayerInputComponent(Input);

    auto* EIC = Cast<UEnhancedInputComponent>(Input);
    if (!EIC)
    {
        UE_LOG(LogTemp, Error, TEXT("EnhancedInputComponent manquante sur PlayerCamera !"));
        return;
    }

	auto Bind = [&](UInputAction* Action, ETriggerEvent Event, auto Obj, auto Func)
	{
		if (Action && Obj)
		{
			EIC->BindAction(Action, Event, Obj.Get(), Func);
		}
	};

    // ----------------------------------------------------
    // Movement System
    // ----------------------------------------------------
    if (MovementSystem)
    {
        Bind(MoveAction, ETriggerEvent::Triggered, MovementSystem, &UCameraMovementSystem::HandleMoveInput);
        Bind(MoveAction, ETriggerEvent::Completed, MovementSystem, &UCameraMovementSystem::HandleMoveReleased);
        Bind(ZoomAction, ETriggerEvent::Triggered, MovementSystem, &UCameraMovementSystem::HandleZoomInput);
        Bind(RotateHorizontalAction, ETriggerEvent::Triggered, MovementSystem, &UCameraMovementSystem::HandleRotateHorizontal);
        Bind(RotateVerticalAction, ETriggerEvent::Triggered, MovementSystem, &UCameraMovementSystem::HandleRotateVertical);
        Bind(EnableRotateAction, ETriggerEvent::Triggered, MovementSystem, &UCameraMovementSystem::HandleRotateToggle);
    }

    // ----------------------------------------------------
    // Selection System
    // ----------------------------------------------------
    if (SelectionSystem)
    {
        Bind(SelectAction, ETriggerEvent::Started, SelectionSystem, &UCameraSelectionSystem::HandleSelectionPressed);
        Bind(SelectAction, ETriggerEvent::Completed, SelectionSystem, &UCameraSelectionSystem::HandleSelectionReleased);
        Bind(SelectHoldAction, ETriggerEvent::Triggered, SelectionSystem, &UCameraSelectionSystem::HandleSelectionHold);
        Bind(DoubleTapAction, ETriggerEvent::Completed, SelectionSystem, &UCameraSelectionSystem::HandleSelectAll);
    }

    // ----------------------------------------------------
    // Command System
    // ----------------------------------------------------
    if (CommandSystem)
    {
        Bind(CommandAction, ETriggerEvent::Started, CommandSystem, &UCameraCommandSystem::HandleCommandActionStarted);
        Bind(CommandAction, ETriggerEvent::Completed, CommandSystem, &UCameraCommandSystem::HandleCommandActionCompleted);
        Bind(AltCommandAction, ETriggerEvent::Started, CommandSystem, &UCameraCommandSystem::HandleAltPressed);
        Bind(AltCommandAction, ETriggerEvent::Completed, CommandSystem, &UCameraCommandSystem::HandleAltReleased);
        Bind(CancelAction, ETriggerEvent::Triggered, CommandSystem, &UCameraCommandSystem::CancelPatrolCreation);
    }

    // ----------------------------------------------------
    // Spawn System
    // ----------------------------------------------------
    if (SpawnSystem)
    {
        Bind(SpawnUnitAction, ETriggerEvent::Completed, SpawnSystem, &UCameraSpawnSystem::HandleSpawnInput);
    }
}

void APlayerCamera::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();
    Player = Cast<APlayerController>(GetController());
}

// ---------------------------------------------------------
// SYSTEM INITIALIZATION
// ---------------------------------------------------------
void APlayerCamera::InitializeSystems()
{
    // 1. Création des instances
	
    if (!PreviewSystem && PreviewSystemClass)
    {
        PreviewSystem = CreateSystem<UCameraPreviewSystem>(PreviewSystemClass);
        PreviewSystem->Init(this);
    }

    if (!MovementSystem && MovementSystemClass)
    {
        MovementSystem = CreateSystem<UCameraMovementSystem>(MovementSystemClass);
        MovementSystem->Init(this);
    }

    if (!CommandSystem && CommandSystemClass)
    {
        CommandSystem = CreateSystem<UCameraCommandSystem>(CommandSystemClass);
        CommandSystem->Init(this);
    }

    if (!SpawnSystem && SpawnSystemClass)
    {
        SpawnSystem = CreateSystem<UCameraSpawnSystem>(SpawnSystemClass);
        SpawnSystem->Init(this);
    }

    if (!SelectionSystem && SelectionSystemClass)
    {
        SelectionSystem = CreateSystem<UCameraSelectionSystem>(SelectionSystemClass);
        SelectionSystem->Init(this);
    }

    // 2. Injection des dépendances (Cross-dependency)
    
    if (CommandSystem)
    {
        if (PreviewSystem)
        	CommandSystem->SetPreviewSystem(PreviewSystem);
    	
        if (SpawnSystem)
        	CommandSystem->SetSpawnSystem(SpawnSystem);
    }

    if (SpawnSystem)
    {
        if (PreviewSystem)
        	SpawnSystem->SetPreviewSystem(PreviewSystem);
    	
        if (CommandSystem)
        	SpawnSystem->SetCommandSystem(CommandSystem);
    }

    if (PreviewSystem)
    {
        if (SpawnSystem)
        	PreviewSystem->SetSpawnSystem(SpawnSystem);
    }

    if (SelectionSystem)
    {
        if (CommandSystem)
        	SelectionSystem->SetCommandSystem(CommandSystem);
    	
        if (SpawnSystem)
        	SelectionSystem->SetSpawnSystem(SpawnSystem);
    }
}

void APlayerCamera::UnPossessed()
{
    Super::UnPossessed();

    if (auto PC = Cast<APlayerController>(GetController()))
    {
        if (auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (InputMapping)
            	Sub->RemoveMappingContext(InputMapping);
        }
    }
}