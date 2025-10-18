#include "JupiterPlugin/Public/Player/PlayerCamera.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystemInterface.h"
#include "EnhancedInputSubsystems.h"
#include "Player/PlayerControllerRts.h"
#include "Player/Selections/SelectionBox.h"
#include "Player/Selections/SphereRadius.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitFormationComponent.h"
#include "Components/UnitSpawnComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/AssetManager.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interfaces/Selectable.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/RandomStream.h"
#include "Units/SoldierRts.h"
#include "Utilities/PreviewPoseMesh.h"


//----------------------- Setup ----------------------------
#pragma region Setup

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
}

UUnitSelectionComponent* APlayerCamera::GetSelectionComponentChecked() const
{
    if (SelectionComponent)
		return SelectionComponent;

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
	
	if(!Player) return;

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
	TargetRotation = FRotator(Rotation.Pitch + -50.f,  Rotation.Yaw, 0.f);
}

void APlayerCamera::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	auto* EnhancedInput{ Cast<UEnhancedInputComponent>(Input) };
	if (IsValid(EnhancedInput))
	{
		UE_LOG(LogTemp, Display, TEXT("EnhancedInputComponent is valid"));

		//MOVEMENTS
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_OnMove);
		EnhancedInput->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_Zoom);
		
		//ROTATION
		EnhancedInput->BindAction(EnableRotateAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_EnableRotate);
		EnhancedInput->BindAction(RotateHorizontalAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_RotateHorizontal);
		EnhancedInput->BindAction(RotateVerticalAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_RotateVertical);
		
		//SELECTION
		EnhancedInput->BindAction(SelectAction, ETriggerEvent::Started, this, &APlayerCamera::Input_SquareSelection);
		
		EnhancedInput->BindAction(SelectAction, ETriggerEvent::Completed, this, &APlayerCamera::Input_LeftMouseReleased);
		EnhancedInput->BindAction(SelectHoldAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_LeftMouseInputHold);
		
		EnhancedInput->BindAction(DoubleTap, ETriggerEvent::Completed, this, &APlayerCamera::Input_SelectAllUnitType);
		
		
		//COMMANDS
                EnhancedInput->BindAction(CommandAction, ETriggerEvent::Started, this, &APlayerCamera::HandleCommandActionStarted);
		EnhancedInput->BindAction(CommandAction, ETriggerEvent::Completed, this, &APlayerCamera::Command);
		
		EnhancedInput->BindAction(AltCommandAction, ETriggerEvent::Started, this, &APlayerCamera::Input_AltFunction);
		EnhancedInput->BindAction(AltCommandAction, ETriggerEvent::Completed, this, &APlayerCamera::Input_AltFunctionRelease);
		EnhancedInput->BindAction(AltCommandActionTrigger, ETriggerEvent::Triggered, this, &APlayerCamera::Input_AltFunctionHold);
		
		EnhancedInput->BindAction(PatrolCommandAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_PatrolZone);
		EnhancedInput->BindAction(DeleteCommandAction, ETriggerEvent::Triggered, this, &APlayerCamera::Input_OnDestroySelected);
		
		//SPAWN
		EnhancedInput->BindAction(SpawnUnitAction, ETriggerEvent::Completed, this, &APlayerCamera::Input_OnSpawnUnits);
		
	}else
	{
		UE_LOG(LogTemp, Warning, TEXT("EnhancedInputComponent is NOT valid"));
	}
}

#pragma endregion

void APlayerCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(!Player) return;
	
	EdgeScroll();
	CameraBounds();
	
	//const FVector InterpolatedLocation = UKismetMathLibrary::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, CameraSpeed);
	//SetActorLocation(FVector(InterpolatedLocation.X, InterpolatedLocation.Y, 0.f));

	const float InterpolatedZoom = UKismetMathLibrary::FInterpTo(SpringArm->TargetArmLength, TargetZoom, DeltaTime, ZoomSpeed);
	SpringArm->TargetArmLength = InterpolatedZoom;

	const FRotator InterpolatedRotation = UKismetMathLibrary::RInterpTo(SpringArm->GetRelativeRotation(), TargetRotation, DeltaTime, CameraSpeed);
	SpringArm->SetRelativeRotation(InterpolatedRotation);

	if (Player && (Player->GetInputKeyTimeDown(EKeys::LeftAlt) || Player->GetInputKeyTimeDown(EKeys::RightAlt)))
	{
		bAltIsPressed = true;
	}
	else
	{
		bAltIsPressed = false;
	}

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

	auto* NewPlayer{ Cast<APlayerController>(GetController()) };
	if (IsValid(NewPlayer))
	{
		auto* InputSubsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(NewPlayer->GetLocalPlayer()) };
		if (IsValid(InputSubsystem))
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
	const auto* PreviousPlayer{ Cast<APlayerController>(PreviousController) };
	if (IsValid(PreviousPlayer))
	{
		auto* InputSubsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PreviousPlayer->GetLocalPlayer()) };
		if (IsValid(InputSubsystem))
		{
			InputSubsystem->RemoveMappingContext(InputMappingContext);
		}
	}

	auto* NewPlayer{ Cast<APlayerController>(GetController()) };
	if (IsValid(NewPlayer))
	{
		NewPlayer->InputYawScale_DEPRECATED = 1.0f;
		NewPlayer->InputPitchScale_DEPRECATED = 1.0f;
		NewPlayer->InputRollScale_DEPRECATED = 1.0f;

		auto* InputSubsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(NewPlayer->GetLocalPlayer()) };
		if (IsValid(InputSubsystem))
		{
			UE_LOG(LogTemp, Display, TEXT("InputSubsystem is valid"));

			FModifyContextOptions Options;
			Options.bNotifyUserSettings = true;

			InputSubsystem->AddMappingContext(InputMappingContext, 0, Options);

			UE_LOG(LogTemp, Warning, TEXT("Adding Jupiter Mapping Context"));

			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("Adding Jupiter Mapping Context"));
		}else
		{
			UE_LOG(LogTemp, Warning, TEXT("InputSubsystem is NOT valid"));
		}
	}

	Super::NotifyControllerChanged();
}

// ------------------- Camera Movement ---------------------
#pragma region Camera Movement Input

// MOVEMENT
void APlayerCamera::Input_OnMove(const FInputActionValue& ActionValue)
{
	FVector2d InputVector = ActionValue.Get<FVector2D>();

	FVector MovementVector = FVector();
	MovementVector += SpringArm->GetForwardVector() * InputVector.Y;
	MovementVector += SpringArm->GetRightVector() * InputVector.X;
	MovementVector *= CameraSpeed * GetWorld()->GetDeltaSeconds();
	MovementVector.Z = 0;
	
	FVector NextPosition = GetActorLocation() + MovementVector;
	SetActorLocation(NextPosition);
	GetTerrainPosition(NextPosition);
}

// ZOOM
void APlayerCamera::Input_Zoom(const FInputActionValue& ActionValue)
{
	float Value = ActionValue.Get<float>();

	if(Value == 0.f) return;

	const float Zoom = Value * 100.f;
	TargetZoom = FMath::Clamp(Zoom + TargetZoom, MinZoom, MaxZoom);
}

// ROTATION
void APlayerCamera::Input_RotateHorizontal(const FInputActionValue& ActionValue)
{
	float Value = ActionValue.Get<float>() * RotateSpeed * GetWorld()->GetDeltaSeconds();
	
	if(Value == 0.f || !CanRotate) return;
	
	TargetRotation = UKismetMathLibrary::ComposeRotators(TargetRotation, FRotator(0.f, Value, 0.f));
}

void APlayerCamera::Input_RotateVertical(const FInputActionValue& ActionValue)
{
	float Value = ActionValue.Get<float>() * RotateSpeed * GetWorld()->GetDeltaSeconds();

	if(Value == 0.f || !CanRotate) return;

	TargetRotation = UKismetMathLibrary::ComposeRotators(TargetRotation, FRotator(Value, 0.f, 0.f));
}

void APlayerCamera::Input_EnableRotate(const FInputActionValue& ActionValue)
{
	bool Value = ActionValue.Get<bool>();
	CanRotate = Value;
}

#pragma endregion

// ------------------- Camera Utility ----------------------
#pragma region Movement Utiliti

void APlayerCamera::GetTerrainPosition(FVector& TerrainPosition) const
{
	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	
	FVector StartLocation = TerrainPosition;
	StartLocation.Z += 10000.f;
	FVector EndLocation = TerrainPosition;
	EndLocation.Z -= 10000.f;

	if (UWorld* WorldContext = GetWorld())
	{
		if (WorldContext->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, ECC_Visibility, CollisionParams))
		{
			TerrainPosition = Hit.ImpactPoint;
		}
	}
}

void APlayerCamera::EdgeScroll()
{
	if (!CanEdgeScroll) return;
	
	if (UWorld* WorldContext = GetWorld())
	{
		FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(WorldContext);
		const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(WorldContext);
		MousePosition = MousePosition * UWidgetLayoutLibrary::GetViewportScale(WorldContext);

		MousePosition.X = MousePosition.X / ViewportSize.X;
		MousePosition.Y = MousePosition.Y / ViewportSize.Y;

		//Right/Left
		if(MousePosition.X > 0.98f && MousePosition.X > 1.f)
		{
			Input_OnMove(EdgeScrollSpeed);
		}
		else if(MousePosition.X < 0.02f && MousePosition.X < 0.f)
		{
			Input_OnMove(-EdgeScrollSpeed);	
		}
		//Forward/BackWard
		if(MousePosition.Y > 0.98f && MousePosition.Y > 1.f)
		{
			Input_OnMove(-EdgeScrollSpeed);
		}
		else if(MousePosition.Y < 0.02f && MousePosition.Y < 0.f)
		{
			Input_OnMove(EdgeScrollSpeed);
		}	
	}
}

void APlayerCamera::CameraBounds()
{
	float NewPitch = TargetRotation.Pitch;
	if(TargetRotation.Pitch < (RotatePitchMax * -1.f))
	{
		NewPitch = (RotatePitchMax * -1.f);
	}
	else if(TargetRotation.Pitch > (RotatePitchMin * -1.f))
	{
		NewPitch = (RotatePitchMin * -1.f);
	}

	TargetRotation = FRotator(NewPitch, TargetRotation.Yaw, 0.f);
}

#pragma endregion

// ------------------- Selection ---------------------------
#pragma region Selection

// Left Click
void APlayerCamera::Input_LeftMouseReleased()
{
	if(!MouseProjectionIsGrounded) return;

	HandleLeftMouse(IE_Released, 0.f);
}

void APlayerCamera::Input_LeftMouseInputHold(const FInputActionValue& ActionValue)
{
	if(!MouseProjectionIsGrounded) return;
	
	float Value = ActionValue.Get<float>();
	HandleLeftMouse(IE_Repeat, Value);
}

void APlayerCamera::Input_SquareSelection()
{
        if(!Player) return;

        BoxSelect = false;

        if (SpawnComponent)
        {
            SpawnComponent->SetUnitToSpawn(nullptr);
        }

        HidePreview();

        if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
        {
            Selection->Handle_Selection(nullptr);

            FHitResult Hit = Selection->GetMousePositionOnTerrain();
            MouseProjectionIsGrounded = Hit.bBlockingHit;

            if(MouseProjectionIsGrounded)
            {
                LeftMouseHitLocation = Hit.Location;
            }
        }
        else
        {
            MouseProjectionIsGrounded = false;
        }
}

//---------------- Left Click Utilities
void APlayerCamera::HandleLeftMouse(EInputEvent InputEvent, float Value)
{
        if (!Player || !MouseProjectionIsGrounded) return;

        switch (InputEvent)
        {
                case IE_Pressed:
                {
                        if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
                        {
                            Selection->Handle_Selection(nullptr);
                            BoxSelect = false;
                            LeftMouseHitLocation = Selection->GetMousePositionOnTerrain().Location;
                            CommandStart();
                        }
                        break;
                }

                case IE_Released:

                        MouseProjectionIsGrounded = false;

			if (BoxSelect && SelectionBox)
			{
                                SelectionBox->End();
                                BoxSelect = false;
                        }
                        else
                        {
                                if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
                                {
                                    Selection->Handle_Selection(GetSelectedObject());
                                }
                        }
                        break;

                case IE_Repeat:
                        if (Value == 0.f)
                        {
                                if (SelectionBox)
                                {
                                        SelectionBox->End();
                                }
                                return;
                        }

			if (Player->GetInputKeyTimeDown(EKeys::LeftMouseButton) >= LeftMouseHoldThreshold && SelectionBox)
			{
				if (!BoxSelect)
				{
					SelectionBox->Start(LeftMouseHitLocation, TargetRotation);
					BoxSelect = true;
				}
			}
			break;

		default:
			break;
	}
}

void APlayerCamera::Input_SelectAllUnitType()
{
        TArray<AActor*> SelectedInCameraBound = GetAllActorsOfClassInCameraBound<AActor>(GetWorld(), ASoldierRts::StaticClass());
        TArray<AActor*> ActorsToSelect;
        if (!SelectedInCameraBound.IsEmpty())
        {
                if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
                {
                        const TArray<AActor*> CurrentSelection = Selection->GetSelectedActors();
                        if (Player && !CurrentSelection.IsEmpty())
                        {
                                const ETeams Team = ISelectable::Execute_GetCurrentTeam(CurrentSelection[0]);
                                for (AActor* Solider : SelectedInCameraBound)
                                {
                                        if (Solider && Team == ISelectable::Execute_GetCurrentTeam(Solider))
                                        {
                                                ActorsToSelect.Add(Solider);
                                        }
                                }

                                if (!ActorsToSelect.IsEmpty())
                                {
                                        Selection->Handle_Selection(ActorsToSelect);
                                }
                        }
                }
        }
}

AActor* APlayerCamera::GetSelectedObject()
{
	if (UWorld* World = GetWorld())
	{
		FVector WorldLocation, WorldDirection;

		Player->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
		FVector End = WorldDirection * 1000000.0f + WorldLocation;

		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(this);

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, WorldLocation, End, ECC_Visibility, CollisionParams))
		{
			if (AActor* HitActor = Hit.GetActor())
			{
				return HitActor;
			}
		}
	}

	return nullptr;
}


// Alt Click
void APlayerCamera::Input_AltFunction()
{
	if(!MouseProjectionIsGrounded) return;
	
	HandleAltRightMouse(EInputEvent::IE_Pressed, 1.f);
	CommandStart();
}

void APlayerCamera::Input_AltFunctionRelease()
{
	HandleAltRightMouse(EInputEvent::IE_Released, 0.f);
}

void APlayerCamera::Input_AltFunctionHold(const FInputActionValue& ActionValue)
{
	if(!MouseProjectionIsGrounded) return;

	float Value = ActionValue.Get<float>();
	HandleAltRightMouse(EInputEvent::IE_Repeat, Value);
}

void APlayerCamera::HandleAltRightMouse(EInputEvent InputEvent, float Value)
{
	if (!Player) return;
	FHitResult Result;

        switch (InputEvent)
        {
                case IE_Pressed:
                {
                        if (!bAltIsPressed) return;

                        if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
                        {
                            Result = Selection->GetMousePositionOnTerrain();
                            LeftMouseHitLocation = Result.Location;
                        }
                        else
                        {
                            return;
                        }

                        break;
                }
		case IE_Released:
		{
			if (SphereRadiusEnable && SphereRadius)
			{
				SphereRadius->End();
				SphereRadiusEnable = false;
                                if (OrderComponent)
                                {
                                    OrderComponent->IssueOrder(CreateCommandData(ECommandType::CommandPatrol, nullptr, SphereRadius->GetRadius()));
                                }
			}
			break;	
		}
                case IE_Repeat:  // Correspond au "Hold"
                {
                        if (!bAltIsPressed)
                        {
                                if (SphereRadius)
                                {
                                        SphereRadius->End();
                                }
                                SphereRadiusEnable = false;
                                return;
                        }

                        if (Value == 0.f)
                        {
                                if (SphereRadius)
                                {
                                        SphereRadius->End();
                                }
                                return;
                        }

			if (Player->GetInputKeyTimeDown(EKeys::RightMouseButton) >= LeftMouseHoldThreshold && SphereRadius)
			{
				if (!SphereRadiusEnable)
				{
					SphereRadius->Start(LeftMouseHitLocation, TargetRotation);
					SphereRadiusEnable = true;
				}
			}
			break;
		}

	default:
		break;
	}
}

//----------------
void APlayerCamera::CreateSelectionBox()
{
	if(SelectionBox) return;

	if(UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Instigator = this;
		SpawnParams.Owner = this;
		SelectionBox = World->SpawnActor<ASelectionBox>(SelectionBoxClass,FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if(SelectionBox)
		{
			SelectionBox->SetOwner(this);
		}
	}
}

void APlayerCamera::CreateSphereRadius()
{
	if(UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Instigator = this;
		SpawnParams.Owner = this;
		SphereRadius = World->SpawnActor<ASphereRadius>(SphereRadiusClass,FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if(SphereRadius)
		{
			SphereRadius->SetOwner(this);
		}
	}
}

#pragma endregion

// ------------------- Command  ---------------------
#pragma region Command

void APlayerCamera::Input_PatrolZone(const FInputActionValue& ActionValue)
{
        float Value = ActionValue.Get<float>();

        if(!bAltIsPressed)
        {
                if (SphereRadius)
                {
                        SphereRadius->End();
                }
                SphereRadiusEnable = false;
                return;
        }

        if (!Player || Value == 0.f)
        {
                if (SphereRadius)
                {
                        SphereRadius->End();
                }
                return;
        }

	if (Player->GetInputKeyTimeDown(EKeys::RightMouseButton) >= LeftMouseHoldThreshold && SphereRadius)
	{
		if (!SphereRadiusEnable && SphereRadius)
		{
			SphereRadius->Start(LeftMouseHitLocation, TargetRotation);
			SphereRadiusEnable = true;
		}
	}
}

void APlayerCamera::Input_OnDestroySelected()
{
        if(!Player) return;

        if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
        {
                TArray<AActor*> SelectedActors = Selection->GetSelectedActors();
                if(SelectedActors.Num() > 0)
                {
                        Server_DestroyActor(SelectedActors);
                }
        }
}

void APlayerCamera::Server_DestroyActor_Implementation(const TArray<AActor*>& ActorToDestroy)
{
        for (AActor* Actor : ActorToDestroy)
        {
                if (Actor) Actor->Destroy();
        }
}

//----------------
void APlayerCamera::HandleCommandActionStarted()
{
        if (!Player || bIsInSpawnUnits)
        {
                bIsCommandActionHeld = false;
                return;
        }

        bIsCommandActionHeld = true;

        if (const UWorld* World = GetWorld())
        {
                CommandActionHoldStartTime = World->GetTimeSeconds();
        }
        else
        {
                CommandActionHoldStartTime = 0.f;
        }

        CommandRotationInitialDirection = FVector::ZeroVector;
        CommandRotationBase = FRotator(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);
        CurrentCommandPreviewRotation = CommandRotationBase;
        bCommandRotationPreviewActive = false;
        bCommandPreviewVisible = false;
        CommandPreviewInstanceCount = 0;

        CommandStart();

        CommandPreviewCenter = CommandLocation;
}

void APlayerCamera::CommandStart()
{
        if (!Player || bIsInSpawnUnits) return;

        if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
        {
                const FVector MouseLocation = Selection->GetMousePositionOnTerrain().Location;
                CommandLocation = FVector(MouseLocation.X, MouseLocation.Y, MouseLocation.Z);
        }
}

void APlayerCamera::Command()
{
        if (!Player || bIsInSpawnUnits || bAltIsPressed)
        {
                bIsCommandActionHeld = false;
                StopCommandPreview();
                return;
        }

        FHitResult Hit;
        if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
        {
                Hit = Selection->GetMousePositionOnTerrain();
        }
        else
        {
                bIsCommandActionHeld = false;
                StopCommandPreview();
                return;
        }

        AActor* ActorEnemy = GetSelectedObject();

        if (ActorEnemy && ActorEnemy->Implements<USelectable>())
        {
                if (OrderComponent)
                {
                        OrderComponent->IssueOrder(CreateCommandData(CommandAttack, ActorEnemy));
                }

                bIsCommandActionHeld = false;
                StopCommandPreview();
                return;
        }

        const bool bHasValidGround = Hit.bBlockingHit;
        const bool bUseRotationPreview = bCommandRotationPreviewActive;

        if (!bHasValidGround && !bUseRotationPreview)
        {
                bIsCommandActionHeld = false;
                StopCommandPreview();
                return;
        }

        const FVector TargetLocation = bUseRotationPreview && CommandPreviewCenter != FVector::ZeroVector ? CommandPreviewCenter : Hit.Location;

        if (OrderComponent)
        {
                const FRotator DefaultRotation(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);
                const FRotator FinalRotation = bUseRotationPreview ? CurrentCommandPreviewRotation : DefaultRotation;

                FCommandData CommandData(Player, TargetLocation, FinalRotation, CommandMove);
                CommandData.SourceLocation = TargetLocation;
                CommandData.Rotation = FinalRotation;

                OrderComponent->IssueOrder(CommandData);
        }

        bIsCommandActionHeld = false;
        StopCommandPreview();
}

FCommandData APlayerCamera::CreateCommandData(const ECommandType Type, AActor* Enemy, const float Radius) const
{
	if (!Player) return FCommandData();

	if (Type == CommandAttack)
	{
		return FCommandData(Player, CommandLocation, CameraComponent->GetComponentRotation(), Type, Enemy);
	}
	
	if (Type == CommandPatrol) 
	{
		return FCommandData(Player, CommandLocation, CameraComponent->GetComponentRotation(), Type, nullptr, Radius * 2.f);
	}
	
	return FCommandData(Player, CommandLocation, CameraComponent->GetComponentRotation(), Type);
}


#pragma endregion

// -------------------  Spawn Units  ---------------------
#pragma region Spawn Units

void APlayerCamera::CreatePreviewMesh()
{
        if (!Player || !Player->IsLocalController() || !PreviewUnitsClass)
        {
                return;
        }

        EnsurePreviewUnit();

        if (SpawnComponent)
        {
                ShowUnitPreview(SpawnComponent->GetUnitToSpawn());
        }
}

void APlayerCamera::Input_OnSpawnUnits()
{
        if (!bIsInSpawnUnits || !HasPreviewUnits())
        {
                return;
        }

        if (PreviewUnit && PreviewUnit->GetIsValidPlacement() && SpawnComponent)
        {
                const FVector SpawnLocation = SpawnPreviewCenter;
                const FRotator SpawnRotation = bSpawnRotationPreviewActive ? CurrentSpawnPreviewRotation : FRotator::ZeroRotator;
                SpawnComponent->SpawnUnitsWithTransform(SpawnLocation, SpawnRotation);
        }

        bSpawnRotationHoldActive = false;
        bSpawnRotationPreviewActive = false;
}

void APlayerCamera::ShowUnitPreview(TSubclassOf<ASoldierRts> NewUnitClass)
{
        if (!HasPreviewUnits())
        {
                return;
        }

        if (NewUnitClass)
        {
                if (ASoldierRts* DefaultSoldier = NewUnitClass->GetDefaultObject<ASoldierRts>())
                {
                        const int32 InstanceCount = SpawnComponent ? GetEffectiveSpawnCount() : 1;

                        if (USkeletalMeshComponent* MeshComponent = DefaultSoldier->GetMesh())
                        {
                                if (USkeletalMesh* SkeletalMesh = MeshComponent->GetSkeletalMeshAsset())
                                {
                                        bIsInSpawnUnits = true;
                                        bSpawnRotationPreviewActive = false;
                                        bSpawnRotationHoldActive = false;
                                        SpawnRotationInitialDirection = FVector::ZeroVector;
                                        SpawnRotationBase = FRotator(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);
                                        CurrentSpawnPreviewRotation = SpawnRotationBase;
                                        PreviewUnit->ShowPreview(SkeletalMesh, DefaultSoldier->GetCapsuleComponent()->GetRelativeScale3D(), InstanceCount);
                                        if (GetSelectionComponentChecked())
                                        {
                                                PreviewFollowMouse();
                                                return;
                                        }
                                        HidePreview();
                                        return;
                                }
                        }

                        if (UStaticMeshComponent* StaticMeshComponent = DefaultSoldier->FindComponentByClass<UStaticMeshComponent>())
                        {
                                if (UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh())
                                {
                                        bIsInSpawnUnits = true;
                                        bSpawnRotationPreviewActive = false;
                                        bSpawnRotationHoldActive = false;
                                        SpawnRotationInitialDirection = FVector::ZeroVector;
                                        SpawnRotationBase = FRotator(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);
                                        CurrentSpawnPreviewRotation = SpawnRotationBase;
                                        PreviewUnit->ShowPreview(StaticMesh, StaticMeshComponent->GetRelativeScale3D(), InstanceCount);
                                        if (GetSelectionComponentChecked())
                                        {
                                                PreviewFollowMouse();
                                                return;
                                        }
                                        HidePreview();
                                        return;
                                }
                        }
                }
        }

        HidePreview();
}

void APlayerCamera::HidePreview()
{
        if (PreviewUnit)
        {
                PreviewUnit->HidePreview();
        }
        bIsInSpawnUnits = false;
        bSpawnRotationPreviewActive = false;
        bSpawnRotationHoldActive = false;
}

void APlayerCamera::PreviewFollowMouse()
{

        if (bIsInSpawnUnits && HasPreviewUnits() && Player)
        {

                FHitResult MouseHit;
                if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
                {
                        MouseHit = Selection->GetMousePositionOnTerrain();
                }
                else
                {
                        return;
                }
                const FVector MouseLocation = MouseHit.Location;

                if (MouseLocation == FVector::ZeroVector)
                {
                        return;
                }

                const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
                const bool bLeftMouseDown = Player->IsInputKeyDown(EKeys::LeftMouseButton);

                if (Player->WasInputKeyJustPressed(EKeys::LeftMouseButton))
                {
                        bSpawnRotationHoldActive = true;
                        SpawnRotationHoldStartTime = CurrentTime;
                        bSpawnRotationPreviewActive = false;
                        SpawnPreviewCenter = MouseLocation;
                        SpawnRotationInitialDirection = FVector::ZeroVector;
                        SpawnRotationBase = FRotator(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);
                        CurrentSpawnPreviewRotation = SpawnRotationBase;
                }
                else if (!bLeftMouseDown)
                {
                        bSpawnRotationHoldActive = false;
                }

                if (!bSpawnRotationPreviewActive)
                {
                        const FRotator MouseRotation(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);
                        CurrentSpawnPreviewRotation = MouseRotation;
                        SpawnRotationBase = MouseRotation;
                        SpawnPreviewCenter = MouseLocation;
                }

                if (bSpawnRotationHoldActive && !bSpawnRotationPreviewActive)
                {
                        if (CurrentTime - SpawnRotationHoldStartTime >= RotationPreviewHoldTime)
                        {
                                bSpawnRotationPreviewActive = true;
                                SpawnPreviewCenter = MouseLocation;
                                SpawnRotationBase = CurrentSpawnPreviewRotation;

                                FVector InitialDirection = MouseLocation - SpawnPreviewCenter;
                                InitialDirection.Z = 0.f;
                                if (!InitialDirection.Normalize())
                                {
                                        InitialDirection = SpawnRotationBase.Vector();
                                        InitialDirection.Z = 0.f;
                                        if (!InitialDirection.Normalize())
                                        {
                                                InitialDirection = FVector::XAxisVector;
                                        }
                                }

                                SpawnRotationInitialDirection = InitialDirection;
                        }
                }

                if (bSpawnRotationPreviewActive)
                {
                        CurrentSpawnPreviewRotation = ComputeRotationFromMouseDelta(SpawnPreviewCenter, SpawnRotationInitialDirection, SpawnRotationBase, MouseLocation);
                }

                UpdatePreviewTransforms(SpawnPreviewCenter, CurrentSpawnPreviewRotation);
        }
}

void APlayerCamera::UpdateCommandPreview()
{
        if (!bIsCommandActionHeld)
        {
                return;
        }

        if (!Player)
        {
                bIsCommandActionHeld = false;
                StopCommandPreview();
                return;
        }

        UUnitSelectionComponent* Selection = GetSelectionComponentChecked();
        if (!Selection)
        {
                StopCommandPreview();
                return;
        }

        const UWorld* World = GetWorld();
        const float CurrentTime = World ? World->GetTimeSeconds() : 0.f;

        FHitResult MouseHit = Selection->GetMousePositionOnTerrain();
        const FVector MouseLocation = MouseHit.Location;

        if (!bCommandRotationPreviewActive)
        {
                if (CurrentTime - CommandActionHoldStartTime >= RotationPreviewHoldTime)
                {
                        BeginCommandRotationPreview(MouseLocation);
                }
                else
                {
                        return;
                }
        }

        if (!bCommandRotationPreviewActive)
        {
                return;
        }

        CurrentCommandPreviewRotation = ComputeRotationFromMouseDelta(CommandPreviewCenter, CommandRotationInitialDirection, CommandRotationBase, MouseLocation);

        if (!PreviewUnit)
        {
                return;
        }

        TArray<FTransform> InstanceTransforms;
        BuildCommandPreviewTransforms(CommandPreviewCenter, CurrentCommandPreviewRotation, InstanceTransforms);

        if (!InstanceTransforms.IsEmpty())
        {
                PreviewUnit->UpdateInstances(InstanceTransforms);
        }
}

void APlayerCamera::BeginCommandRotationPreview(const FVector& MouseLocation)
{
        if (!SelectionComponent)
        {
                return;
        }

        const TArray<AActor*> SelectedUnits = SelectionComponent->GetSelectedActors();
        if (SelectedUnits.IsEmpty())
        {
                return;
        }

        if (CommandPreviewCenter == FVector::ZeroVector && MouseLocation != FVector::ZeroVector)
        {
                CommandPreviewCenter = MouseLocation;
        }

        if (!EnsureCommandPreviewMesh(SelectedUnits))
        {
                return;
        }

        bCommandRotationPreviewActive = true;
        CommandRotationBase = FRotator(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);
        CurrentCommandPreviewRotation = CommandRotationBase;

        FVector InitialDirection = MouseLocation - CommandPreviewCenter;
        InitialDirection.Z = 0.f;
        if (!InitialDirection.Normalize())
        {
                InitialDirection = CommandRotationBase.Vector();
                InitialDirection.Z = 0.f;
                if (!InitialDirection.Normalize())
                {
                        InitialDirection = FVector::XAxisVector;
                }
        }

        CommandRotationInitialDirection = InitialDirection;

        if (PreviewUnit)
        {
                TArray<FTransform> InstanceTransforms;
                BuildCommandPreviewTransforms(CommandPreviewCenter, CurrentCommandPreviewRotation, InstanceTransforms);
                if (!InstanceTransforms.IsEmpty())
                {
                        PreviewUnit->UpdateInstances(InstanceTransforms);
                }
        }
}

void APlayerCamera::StopCommandPreview()
{
        if (bCommandPreviewVisible && PreviewUnit && !bIsInSpawnUnits)
        {
                PreviewUnit->HidePreview();
        }

        bCommandRotationPreviewActive = false;
        bCommandPreviewVisible = false;
        CommandPreviewInstanceCount = 0;
}

bool APlayerCamera::EnsureCommandPreviewMesh(const TArray<AActor*>& SelectedUnits)
{
        if (SelectedUnits.IsEmpty())
        {
                return false;
        }

        EnsurePreviewUnit();

        if (!HasPreviewUnits())
        {
                return false;
        }

        USkeletalMesh* SkeletalMesh = nullptr;
        FVector SkeletalScale = FVector::OneVector;
        UStaticMesh* StaticMesh = nullptr;
        FVector StaticScale = FVector::OneVector;

        for (AActor* Unit : SelectedUnits)
        {
                if (!Unit)
                {
                        continue;
                }

                if (!SkeletalMesh)
                {
                        if (const USkeletalMeshComponent* SkeletalComponent = Unit->FindComponentByClass<USkeletalMeshComponent>())
                        {
                                if (USkeletalMesh* Mesh = SkeletalComponent->GetSkeletalMeshAsset())
                                {
                                        SkeletalMesh = Mesh;
                                        SkeletalScale = SkeletalComponent->GetRelativeScale3D();
                                }
                        }
                }

                if (!StaticMesh)
                {
                        if (const UStaticMeshComponent* StaticComponent = Unit->FindComponentByClass<UStaticMeshComponent>())
                        {
                                if (UStaticMesh* Mesh = StaticComponent->GetStaticMesh())
                                {
                                        StaticMesh = Mesh;
                                        StaticScale = StaticComponent->GetRelativeScale3D();
                                }
                        }
                }

                if (SkeletalMesh || StaticMesh)
                {
                        break;
                }
        }

        const int32 InstanceCount = SelectedUnits.Num();

        if (SkeletalMesh)
        {
                PreviewUnit->ShowPreview(SkeletalMesh, SkeletalScale, InstanceCount);
                bCommandPreviewVisible = true;
                CommandPreviewInstanceCount = InstanceCount;
                return true;
        }

        if (StaticMesh)
        {
                PreviewUnit->ShowPreview(StaticMesh, StaticScale, InstanceCount);
                bCommandPreviewVisible = true;
                CommandPreviewInstanceCount = InstanceCount;
                return true;
        }

        return false;
}

void APlayerCamera::BuildCommandPreviewTransforms(const FVector& CenterLocation, const FRotator& FacingRotation, TArray<FTransform>& OutTransforms) const
{
        OutTransforms.Reset();

        if (!SelectionComponent)
        {
                OutTransforms.Add(FTransform(FacingRotation, CenterLocation));
                return;
        }

        const TArray<AActor*> SelectedUnits = SelectionComponent->GetSelectedActors();
        if (SelectedUnits.IsEmpty())
        {
                OutTransforms.Add(FTransform(FacingRotation, CenterLocation));
                return;
        }

        if (FormationComponent)
        {
                FCommandData BaseCommand(Player, CenterLocation, FacingRotation, ECommandType::CommandMove);
                BaseCommand.SourceLocation = CenterLocation;
                BaseCommand.Location = CenterLocation;
                BaseCommand.Rotation = FacingRotation;

                TArray<FCommandData> Commands;
                FormationComponent->BuildFormationCommands(BaseCommand, SelectedUnits, Commands);

                if (!Commands.IsEmpty())
                {
                        OutTransforms.Reserve(Commands.Num());
                        for (const FCommandData& Command : Commands)
                        {
                                OutTransforms.Add(FTransform(FacingRotation, Command.Location));
                        }
                        return;
                }
        }

        OutTransforms.Reserve(SelectedUnits.Num());
        for (int32 Index = 0; Index < SelectedUnits.Num(); ++Index)
        {
                OutTransforms.Add(FTransform(FacingRotation, CenterLocation));
        }
}

FRotator APlayerCamera::ComputeRotationFromMouseDelta(const FVector& Center, const FVector& InitialDirection, const FRotator& BaseRotation, const FVector& MouseLocation) const
{
        FVector NormalizedInitial = InitialDirection;
        NormalizedInitial.Z = 0.f;
        if (!NormalizedInitial.Normalize())
        {
                return BaseRotation;
        }

        FVector CurrentDirection = MouseLocation - Center;
        CurrentDirection.Z = 0.f;
        if (!CurrentDirection.Normalize())
        {
                return BaseRotation;
        }

        const float InitialAngle = FMath::Atan2(NormalizedInitial.Y, NormalizedInitial.X);
        const float CurrentAngle = FMath::Atan2(CurrentDirection.Y, CurrentDirection.X);
        const float DeltaDegrees = FMath::RadiansToDegrees(CurrentAngle - InitialAngle);

        FRotator Result = BaseRotation;
        Result.Yaw = FMath::UnwindDegrees(Result.Yaw + DeltaDegrees);
        Result.Pitch = 0.f;
        Result.Roll = 0.f;
        return Result;
}

void APlayerCamera::HandleSpawnCountChanged(int32 NewSpawnCount)
{
        RefreshPreviewInstances();
}

void APlayerCamera::HandleSpawnFormationChanged(ESpawnFormation /*NewFormation*/)
{
        RefreshPreviewInstances();
}

void APlayerCamera::HandleCustomFormationDimensionsChanged(FIntPoint /*NewDimensions*/)
{
        RefreshPreviewInstances();
}

void APlayerCamera::EnsurePreviewUnit()
{
        if (!Player || !Player->IsLocalController() || PreviewUnit)
        {
                return;
        }

        if (!PreviewUnitsClass)
        {
                return;
        }

        if (UWorld* World = GetWorld())
        {
                FActorSpawnParameters SpawnParams;
                SpawnParams.Instigator = this;
                SpawnParams.Owner = this;

                if (APreviewPoseMesh* Preview = World->SpawnActor<APreviewPoseMesh>(PreviewUnitsClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams))
                {
                        Preview->SetReplicates(false);
                        Preview->SetOwner(this);
                        Preview->SetActorHiddenInGame(true);
                        PreviewUnit = Preview;
                }
        }
}

void APlayerCamera::UpdatePreviewTransforms(const FVector& CenterLocation, const FRotator& FacingRotation)
{
        if (!HasPreviewUnits() || !PreviewUnit)
        {
                return;
        }

        const int32 SpawnCount = GetEffectiveSpawnCount();
        const float Spacing = SpawnComponent ? FMath::Max(SpawnComponent->GetFormationSpacing(), 0.f) : 0.f;

        TArray<FVector> FormationOffsets;
        BuildPreviewFormationOffsets(SpawnCount, Spacing, FormationOffsets);

        if (FormationOffsets.Num() == 0)
        {
                return;
        }

        TArray<FTransform> InstanceTransforms;
        InstanceTransforms.Reserve(FormationOffsets.Num());

        const FQuat RotationQuat = FRotator(0.f, FacingRotation.Yaw, 0.f).Quaternion();
        for (const FVector& Offset : FormationOffsets)
        {
                const FVector RotatedOffset = RotationQuat.RotateVector(Offset);
                const FVector FormationLocation = CenterLocation + RotatedOffset;
                InstanceTransforms.Add(FTransform(FacingRotation, FormationLocation));
        }

        PreviewUnit->UpdateInstances(InstanceTransforms);
}

void APlayerCamera::RefreshPreviewInstances()
{
        EnsurePreviewUnit();

        if (bIsInSpawnUnits && SpawnComponent)
        {
                ShowUnitPreview(SpawnComponent->GetUnitToSpawn());
                PreviewFollowMouse();
        }
}

void APlayerCamera::BuildPreviewFormationOffsets(int32 SpawnCount, float Spacing, TArray<FVector>& OutOffsets) const
{
        OutOffsets.Reset();

        if (SpawnCount <= 0)
        {
                return;
        }

        OutOffsets.Reserve(SpawnCount);

        const ESpawnFormation Formation = SpawnComponent ? SpawnComponent->GetSpawnFormation() : ESpawnFormation::Square;

        switch (Formation)
        {
                case ESpawnFormation::Line:
                {
                        const float HalfWidth = (SpawnCount - 1) * 0.5f * Spacing;
                        for (int32 Index = 0; Index < SpawnCount; ++Index)
                        {
                                const float OffsetX = (Index * Spacing) - HalfWidth;
                                OutOffsets.Add(FVector(OffsetX, 0.f, 0.f));
                        }
                        break;
                }
                case ESpawnFormation::Column:
                {
                        const float HalfHeight = (SpawnCount - 1) * 0.5f * Spacing;
                        for (int32 Index = 0; Index < SpawnCount; ++Index)
                        {
                                const float OffsetY = (Index * Spacing) - HalfHeight;
                                OutOffsets.Add(FVector(0.f, OffsetY, 0.f));
                        }
                        break;
                }
                case ESpawnFormation::Wedge:
                {
                        int32 UnitsPlaced = 0;
                        int32 Row = 0;
                        while (UnitsPlaced < SpawnCount)
                        {
                                const int32 UnitsInRow = FMath::Min(Row + 1, SpawnCount - UnitsPlaced);
                                const float RowHalfWidth = (UnitsInRow - 1) * 0.5f * Spacing;
                                const float RowOffsetY = Row * Spacing;
                                for (int32 IndexInRow = 0; IndexInRow < UnitsInRow; ++IndexInRow)
                                {
                                        const float OffsetX = (IndexInRow * Spacing) - RowHalfWidth;
                                        OutOffsets.Add(FVector(OffsetX, RowOffsetY, 0.f));
                                }

                                UnitsPlaced += UnitsInRow;
                                ++Row;
                        }
                        break;
                }
                case ESpawnFormation::Blob:
                {
                        OutOffsets.Add(FVector::ZeroVector);

                        if (SpawnCount > 1)
                        {
                                FRandomStream RandomStream(12345);
                                const float MaxRadius = (Spacing <= 0.f) ? 0.f : Spacing * FMath::Sqrt(static_cast<float>(SpawnCount));

                                for (int32 Index = 1; Index < SpawnCount; ++Index)
                                {
                                        const float Angle = RandomStream.FRandRange(0.f, 2.f * PI);
                                        const float Radius = (MaxRadius > 0.f) ? RandomStream.FRandRange(0.f, MaxRadius) : 0.f;
                                        const float OffsetX = Radius * FMath::Cos(Angle);
                                        const float OffsetY = Radius * FMath::Sin(Angle);
                                        OutOffsets.Add(FVector(OffsetX, OffsetY, 0.f));
                                }
                        }
                        break;
                }
                case ESpawnFormation::Custom:
                {
                        const FIntPoint Dimensions = SpawnComponent ? SpawnComponent->GetCustomFormationDimensions() : FIntPoint(1, 1);
                        const int32 Columns = FMath::Max(1, Dimensions.X);
                        const int32 Rows = FMath::Max(1, Dimensions.Y);
                        const float HalfWidth = (Columns - 1) * 0.5f * Spacing;
                        const float HalfHeight = (Rows - 1) * 0.5f * Spacing;

                        for (int32 Index = 0; Index < SpawnCount; ++Index)
                        {
                                const int32 Column = Index % Columns;
                                const int32 RowIndex = Index / Columns;

                                const float OffsetX = (Column * Spacing) - HalfWidth;
                                const float OffsetY = (RowIndex * Spacing) - HalfHeight;
                                OutOffsets.Add(FVector(OffsetX, OffsetY, 0.f));
                        }
                        break;
                }
                case ESpawnFormation::Square:
                default:
                {
                        const int32 Columns = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(SpawnCount))));
                        const int32 Rows = FMath::Max(1, FMath::CeilToInt(static_cast<float>(SpawnCount) / static_cast<float>(Columns)));
                        const float HalfWidth = (Columns - 1) * 0.5f * Spacing;
                        const float HalfHeight = (Rows - 1) * 0.5f * Spacing;

                        for (int32 Index = 0; Index < SpawnCount; ++Index)
                        {
                                const int32 Column = Index % Columns;
                                const int32 RowIndex = Index / Columns;

                                const float OffsetX = (Column * Spacing) - HalfWidth;
                                const float OffsetY = (RowIndex * Spacing) - HalfHeight;
                                OutOffsets.Add(FVector(OffsetX, OffsetY, 0.f));
                        }
                        break;
                }
        }

        if (OutOffsets.Num() > 0 && (Formation == ESpawnFormation::Wedge || Formation == ESpawnFormation::Blob))
        {
                FVector AverageOffset = FVector::ZeroVector;
                for (const FVector& Offset : OutOffsets)
                {
                        AverageOffset += Offset;
                }

                AverageOffset /= static_cast<float>(OutOffsets.Num());

                for (FVector& Offset : OutOffsets)
                {
                        Offset -= FVector(AverageOffset.X, AverageOffset.Y, 0.f);
                }
        }
}

int32 APlayerCamera::GetEffectiveSpawnCount() const
{
        if (!SpawnComponent)
        {
                return 1;
        }

        if (SpawnComponent->GetSpawnFormation() == ESpawnFormation::Custom)
        {
                const FIntPoint Dimensions = SpawnComponent->GetCustomFormationDimensions();
                return FMath::Max(1, Dimensions.X * Dimensions.Y);
        }

        return FMath::Max(SpawnComponent->GetUnitsPerSpawn(), 1);
}

#pragma endregion 




