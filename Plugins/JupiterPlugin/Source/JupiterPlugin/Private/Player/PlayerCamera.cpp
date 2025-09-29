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
#include "Components/UnitSelectionComponent.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitFormationComponent.h"
#include "Components/UnitSpawnComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interfaces/Selectable.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
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
        FormationComponent->OnFormationStateChanged.AddDynamic(OrderComponent, &UUnitOrderComponent::ReapplyCachedFormation);
    }

    if (SpawnComponent)
    {
        SpawnComponent->OnUnitClassChanged.AddDynamic(this, &APlayerCamera::ShowUnitPreview);
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
		EnhancedInput->BindAction(CommandAction, ETriggerEvent::Started, this, &APlayerCamera::CommandStart);
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

	if(PreviewUnits)
		PreviewUnits->Destroy();
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
        if (!Player || bAltIsPressed || bIsInSpawnUnits) return;

        FHitResult Hit;
        if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
        {
                Hit = Selection->GetMousePositionOnTerrain();
        }
        else
        {
                return;
        }
        AActor* ActorEnemy = GetSelectedObject();
	
	if (ActorEnemy && ActorEnemy->Implements<USelectable>())
	{
            if (OrderComponent)
            {
                OrderComponent->IssueOrder(CreateCommandData(CommandAttack, ActorEnemy));
            }
		return;
	}

	if (Hit.bBlockingHit)
            if (OrderComponent)
            {
                OrderComponent->IssueOrder(FCommandData(Player, Hit.Location, GetActorRotation(), CommandMove));
            }
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
	if(PreviewUnits) return;

	if (Player && Player->IsLocalController())
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("PLAYER VALID"));

		if(UWorld* World = GetWorld())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Instigator = this;
			SpawnParams.Owner = this;
		
			PreviewUnits = World->SpawnActor<APreviewPoseMesh>(PreviewUnitsClass,FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

                        if (PreviewUnits)
                        {
                                PreviewUnits->SetReplicates(false);
                                PreviewUnits->SetOwner(this);

                                if (SpawnComponent)
                                {
                                        ShowUnitPreview(SpawnComponent->GetUnitToSpawn());
                                }
                        }
		}	
	}else
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("NO PLAYER"));
	}
}

void APlayerCamera::Input_OnSpawnUnits()
{

	UE_LOG(LogTemp, Warning, TEXT("Inside OnSpawnUnits"));

	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("Input SpawnUnits"));

	
	if (!bIsInSpawnUnits && !PreviewUnits) return;


        if (PreviewUnits->GetIsValidPlacement() && SpawnComponent)
        {
                SpawnComponent->SpawnUnits();
        }
}

void APlayerCamera::ShowUnitPreview(TSubclassOf<ASoldierRts> NewUnitClass)
{
        if (!PreviewUnits)
        {
                return;
        }

        if (NewUnitClass)
        {
                if (ASoldierRts* DefaultSoldier = NewUnitClass->GetDefaultObject<ASoldierRts>())
                {
                        if (USkeletalMeshComponent* MeshComponent = DefaultSoldier->GetMesh())
                        {
                                if (MeshComponent->GetSkeletalMeshAsset())
                                {
                                        bIsInSpawnUnits = true;
                                        PreviewUnits->ShowPreview(MeshComponent->GetSkeletalMeshAsset(), DefaultSoldier->GetCapsuleComponent()->GetRelativeScale3D());
                                        if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
                                        {
                                                PreviewUnits->SetActorLocation(Selection->GetMousePositionOnTerrain().Location);
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
        if (PreviewUnits)
        {
                PreviewUnits->HidePreview();
        }
        bIsInSpawnUnits = false;
}

void APlayerCamera::PreviewFollowMouse()
{

        if (bIsInSpawnUnits && PreviewUnits && Player)
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
                FRotator MouseRotation = FRotator(0,  CameraComponent->GetComponentRotation().Yaw, CameraComponent->GetComponentRotation().Roll);

                if (MouseHit.Location != FVector::ZeroVector)
                {
			PreviewUnits->SetActorLocation(MouseHit.Location);
			PreviewUnits->SetActorRotation(MouseRotation);

			PreviewUnits->CheckIsValidPlacement();
		}
	}
}

#pragma endregion 




