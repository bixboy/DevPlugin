#include "JupiterPlugin/Public/Player/PlayerCamera.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/UnitFormationComponent.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Components/UnitSpawnComponent.h"
#include "Components/UnitPatrolComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystemInterface.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utilities/PreviewPoseMesh.h"

APlayerCamera::APlayerCamera()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
    RootComponent = SceneComponent;

    PawnMovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("PawnMovementComponent"));

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 2000.0f;
    SpringArm->bDoCollisionTest = false;

    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(SpringArm);

    SelectionComponent = CreateDefaultSubobject<UUnitSelectionComponent>(TEXT("SelectionComponent"));
    OrderComponent = CreateDefaultSubobject<UUnitOrderComponent>(TEXT("OrderComponent"));
    FormationComponent = CreateDefaultSubobject<UUnitFormationComponent>(TEXT("FormationComponent"));
    SpawnComponent = CreateDefaultSubobject<UUnitSpawnComponent>(TEXT("SpawnComponent"));
    PatrolComponent = CreateDefaultSubobject<UUnitPatrolComponent>(TEXT("PatrolComponent"));
}

UUnitSelectionComponent* APlayerCamera::GetSelectionComponentChecked() const
{
    if (SelectionComponent)
    {
        return SelectionComponent;
    }

    UUnitSelectionComponent* FoundComponent = FindComponentByClass<UUnitSelectionComponent>();
    if (!FoundComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s is missing a UnitSelectionComponent."), *GetPathName());
        return nullptr;
    }

    const_cast<APlayerCamera*>(this)->SelectionComponent = FoundComponent;
    return FoundComponent;
}

void APlayerCamera::BeginPlay()
{
    Super::BeginPlay();

    Player = Cast<APlayerController>(GetInstigatorController());
    if (!Player)
		return;

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    Player->SetInputMode(InputMode);
    Player->bShowMouseCursor = true;

    CustomInitialized();

    CreateSelectionBox();
    CreateSphereRadius();

    if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
    {
        Selection->CreateHud();
    }

    if (FormationComponent && OrderComponent)
    {
        FormationComponent->OnFormationStateChanged.RemoveDynamic(OrderComponent, &UUnitOrderComponent::ReapplyCachedFormation);
        FormationComponent->OnFormationStateChanged.AddDynamic(OrderComponent, &UUnitOrderComponent::ReapplyCachedFormation);
    }

    if (SpawnComponent)
    {
        SpawnComponent->OnUnitClassChanged.RemoveDynamic(this, &APlayerCamera::ShowUnitPreview);
        SpawnComponent->OnUnitClassChanged.AddDynamic(this, &APlayerCamera::ShowUnitPreview);
        SpawnComponent->OnSpawnCountChanged.RemoveDynamic(this, &APlayerCamera::HandleSpawnCountChanged);
        SpawnComponent->OnSpawnCountChanged.AddDynamic(this, &APlayerCamera::HandleSpawnCountChanged);
        SpawnComponent->OnSpawnFormationChanged.RemoveDynamic(this, &APlayerCamera::HandleSpawnFormationChanged);
        SpawnComponent->OnSpawnFormationChanged.AddDynamic(this, &APlayerCamera::HandleSpawnFormationChanged);
        SpawnComponent->OnCustomFormationDimensionsChanged.RemoveDynamic(this, &APlayerCamera::HandleCustomFormationDimensionsChanged);
        SpawnComponent->OnCustomFormationDimensionsChanged.AddDynamic(this, &APlayerCamera::HandleCustomFormationDimensionsChanged);
    }

    CreatePreviewMesh();
}

void APlayerCamera::CustomInitialized()
{
        TargetLocation = GetActorLocation();
        TargetZoom = 3000.f;

        const FRotator Rotation = SpringArm->GetRelativeRotation();
        TargetRotation = FRotator(Rotation.Pitch + -50.f, Rotation.Yaw, 0.f);
}

void APlayerCamera::SetupPlayerInputComponent(UInputComponent* Input)
{
        Super::SetupPlayerInputComponent(Input);

        if (auto* EnhancedInput = Cast<UEnhancedInputComponent>(Input))
        {
            // MOVEMENTS
            EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_OnMove);
            EnhancedInput->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_Zoom);

            // ROTATION
            EnhancedInput->BindAction(EnableRotateAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_EnableRotate);
            EnhancedInput->BindAction(RotateHorizontalAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_RotateHorizontal);
            EnhancedInput->BindAction(RotateVerticalAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_RotateVertical);

            // SELECTION
            EnhancedInput->BindAction(SelectAction, ETriggerEvent::Started, this, &APlayerCamera::Input_SquareSelection);
            EnhancedInput->BindAction(SelectAction, ETriggerEvent::Completed, this, &APlayerCamera::Input_LeftMouseReleased);
            EnhancedInput->BindAction(SelectHoldAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_LeftMouseInputHold);
            EnhancedInput->BindAction(DoubleTap, ETriggerEvent::Completed, this, &APlayerCamera::Input_SelectAllUnitType);

            // COMMANDS
            EnhancedInput->BindAction(CommandAction, ETriggerEvent::Started, this, &APlayerCamera::HandleCommandActionStarted);
            EnhancedInput->BindAction(CommandAction, ETriggerEvent::Completed, this, &APlayerCamera::Command);
        	
            EnhancedInput->BindAction(AltCommandAction, ETriggerEvent::Started, this, &APlayerCamera::Input_AltFunction);
            EnhancedInput->BindAction(AltCommandAction, ETriggerEvent::Completed, this, &APlayerCamera::Input_AltFunctionRelease);
            EnhancedInput->BindAction(AltCommandActionTrigger, ETriggerEvent::Triggered, this, &APlayerCamera::Input_AltFunctionHold);
        	
            EnhancedInput->BindAction(PatrolCommandAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_PatrolZone);
            EnhancedInput->BindAction(DeleteCommandAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_OnDestroySelected);

            // SPAWN
            EnhancedInput->BindAction(SpawnUnitAction, ETriggerEvent::Completed, this, &APlayerCamera::Input_OnSpawnUnits);
        }
        else
        {
                UE_LOG(LogTemp, Warning, TEXT("EnhancedInputComponent is NOT valid"));
        }
}

void APlayerCamera::Tick(float DeltaTime)
{
        Super::Tick(DeltaTime);

        if (!Player)
        {
                return;
        }

        EdgeScroll();
        CameraBounds();

        const float InterpolatedZoom = UKismetMathLibrary::FInterpTo(SpringArm->TargetArmLength, TargetZoom, DeltaTime, ZoomSpeed);
        SpringArm->TargetArmLength = InterpolatedZoom;

        const FRotator InterpolatedRotation = UKismetMathLibrary::RInterpTo(SpringArm->GetRelativeRotation(), TargetRotation, DeltaTime, CameraSpeed);
        SpringArm->SetRelativeRotation(InterpolatedRotation);

        bAltIsPressed = Player->GetInputKeyTimeDown(EKeys::LeftAlt) > 0.f || Player->GetInputKeyTimeDown(EKeys::RightAlt) > 0.f;

        if (bIsInSpawnUnits)
        {
                PreviewFollowMouse();
        }

        if (bIsCommandActionHeld)
        {
                UpdateCommandPreview();
        }
}

void APlayerCamera::UnPossessed()
{
        Super::UnPossessed();

        if (auto* NewPlayer = Cast<APlayerController>(GetController()))
        {
                if (auto* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(NewPlayer->GetLocalPlayer()))
                {
                        InputSubsystem->RemoveMappingContext(InputMappingContext);
                }
        }

        if (PreviewUnit)
        {
                PreviewUnit->Destroy();
                PreviewUnit = nullptr;
        }
}

void APlayerCamera::NotifyControllerChanged()
{
        if (const auto* PreviousPlayer = Cast<APlayerController>(PreviousController))
        {
                if (auto* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PreviousPlayer->GetLocalPlayer()))
                {
                        InputSubsystem->RemoveMappingContext(InputMappingContext);
                }
        }

        if (auto* NewPlayer = Cast<APlayerController>(GetController()))
        {
                NewPlayer->InputYawScale_DEPRECATED = 1.0f;
                NewPlayer->InputPitchScale_DEPRECATED = 1.0f;
                NewPlayer->InputRollScale_DEPRECATED = 1.0f;

                if (auto* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(NewPlayer->GetLocalPlayer()))
                {
                        FModifyContextOptions Options;
                        Options.bNotifyUserSettings = true;

                        InputSubsystem->AddMappingContext(InputMappingContext, 0, Options);
                        UE_LOG(LogTemp, Display, TEXT("InputSubsystem is valid"));
                        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("Adding Jupiter Mapping Context"));
                }
                else
                {
                        UE_LOG(LogTemp, Warning, TEXT("InputSubsystem is NOT valid"));
                }
        }

        Super::NotifyControllerChanged();
}

