#include "Player/PlayerCamera.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

#include "Camera/CameraComponent.h"
#include "Components/UnitFormationComponent.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitPatrolComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Components/UnitSpawnComponent.h"

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

	if (SpringArm)
	{
		SpringArm->TargetArmLength = TargetZoom;
		SpringArm->SetRelativeRotation(TargetRotation);
	}

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

// ---------------------------------------------------------
// INPUT BINDINGS
// ---------------------------------------------------------
void APlayerCamera::SetupPlayerInputComponent(UInputComponent* Input)
{
    // Ensure systems are initialized BEFORE binding inputs
    InitializeSystems();

    Super::SetupPlayerInputComponent(Input);

    auto* EIC = Cast<UEnhancedInputComponent>(Input);
    if (!EIC)
        return;

    // Raccourci propre
    auto Bind = [&](UInputAction* Action, ETriggerEvent Event, auto Obj, auto Func)
    {
        if (Action)
            EIC->BindAction(Action, Event, Obj, Func);
    };

    // ----------------------------------------------------
    // Movement System
    // ----------------------------------------------------
    if (UCameraMovementSystem* MoveSys = GetMovementSystem())
    {
    	Bind(MoveAction, ETriggerEvent::Triggered, MoveSys, &UCameraMovementSystem::HandleMoveInput);
    	Bind(MoveAction, ETriggerEvent::Completed, MoveSys, &UCameraMovementSystem::HandleMoveReleased);

        Bind(ZoomAction, ETriggerEvent::Triggered, MoveSys, &UCameraMovementSystem::HandleZoomInput);
        Bind(RotateHorizontalAction, ETriggerEvent::Triggered, MoveSys, &UCameraMovementSystem::HandleRotateHorizontal);
        Bind(RotateVerticalAction, ETriggerEvent::Triggered, MoveSys, &UCameraMovementSystem::HandleRotateVertical);
        Bind(EnableRotateAction, ETriggerEvent::Triggered, MoveSys, &UCameraMovementSystem::HandleRotateToggle);
    }

    // ----------------------------------------------------
    // Selection System
    // ----------------------------------------------------
    if (UCameraSelectionSystem* SelSys = GetSelectionSystem())
    {
        Bind(SelectAction, ETriggerEvent::Started, SelSys, &UCameraSelectionSystem::HandleSelectionPressed);
        Bind(SelectAction, ETriggerEvent::Completed, SelSys, &UCameraSelectionSystem::HandleSelectionReleased);
        Bind(SelectHoldAction, ETriggerEvent::Triggered, SelSys, &UCameraSelectionSystem::HandleSelectionHold);
        Bind(DoubleTapAction, ETriggerEvent::Completed, SelSys, &UCameraSelectionSystem::HandleSelectAll);
    }

    // ----------------------------------------------------
    // Command System
    // ----------------------------------------------------
    if (UCameraCommandSystem* CmdSys = GetCommandSystem())
    {
        Bind(CommandAction, ETriggerEvent::Started, CmdSys, &UCameraCommandSystem::HandleCommandActionStarted);
        Bind(CommandAction, ETriggerEvent::Completed, CmdSys, &UCameraCommandSystem::HandleCommandActionCompleted);

        Bind(AltCommandAction, ETriggerEvent::Started, CmdSys, &UCameraCommandSystem::HandleAltPressed);
        Bind(AltCommandAction, ETriggerEvent::Completed, CmdSys, &UCameraCommandSystem::HandleAltReleased);
        //Bind(AltCommandHoldAction, ETriggerEvent::Triggered, CmdSys, &UCameraCommandSystem::HandleAltHold);

        Bind(DeleteAction, ETriggerEvent::Triggered, CmdSys, &UCameraCommandSystem::HandleDestroySelected);
    }

    // ----------------------------------------------------
    // Spawn System
    // ----------------------------------------------------
    if (UCameraSpawnSystem* SpawnSys = GetSpawnSystem())
    {
        Bind(SpawnUnitAction, ETriggerEvent::Completed, SpawnSys, &UCameraSpawnSystem::HandleSpawnInput);
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
    if (PreviewSystemClass && !PreviewSystem)
    {
        PreviewSystem = CreateSystem<UCameraPreviewSystem>(PreviewSystemClass);
        PreviewSystem->Init(this);
    }

    if (MovementSystemClass && !MovementSystem)
    {
        MovementSystem = CreateSystem<UCameraMovementSystem>(MovementSystemClass);
        MovementSystem->Init(this);
    }

    if (CommandSystemClass && !CommandSystem)
    {
        CommandSystem = CreateSystem<UCameraCommandSystem>(CommandSystemClass);
        CommandSystem->Init(this);
    	
        if (PreviewSystem)
        	CommandSystem->SetPreviewSystem(PreviewSystem);
    }

    if (SpawnSystemClass && !SpawnSystem)
    {
        SpawnSystem = CreateSystem<UCameraSpawnSystem>(SpawnSystemClass);
        SpawnSystem->Init(this);
        SpawnSystem->SetPreviewSystem(PreviewSystem);
        SpawnSystem->SetCommandSystem(CommandSystem);
    }
	

    if (SelectionSystemClass && !SelectionSystem)
    {
        SelectionSystem = CreateSystem<UCameraSelectionSystem>(SelectionSystemClass);
        SelectionSystem->Init(this);
        SelectionSystem->SetCommandSystem(CommandSystem);
        SelectionSystem->SetSpawnSystem(SpawnSystem);
    }

    if (CommandSystem && SpawnSystem)
        CommandSystem->SetSpawnSystem(SpawnSystem);
}


// ---------------------------------------------------------
// Cleanup
// ---------------------------------------------------------
void APlayerCamera::UnPossessed()
{
    Super::UnPossessed();

    if (auto PC = Cast<APlayerController>(GetController()))
    {
        if (auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
            Sub->RemoveMappingContext(InputMapping);
    }
}
