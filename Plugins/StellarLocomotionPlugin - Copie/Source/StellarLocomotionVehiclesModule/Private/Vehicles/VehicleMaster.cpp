#include "Vehicles/VehicleMaster.h"
#include "CameraVehicle.h"
#include "CustomPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "VehiclesAnimInstance.h"
#include "Component/SeatComponent.h"
#include "Component/TurretCamera.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

// ------------------- Setup -------------------
#pragma region Setup

AVehicleMaster::AVehicleMaster()
{
    PrimaryActorTick.bCanEverTick = true;

    BaseVehicle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseVehicle"));
    RootComponent = BaseVehicle;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    
    MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    MainCamera->SetupAttachment(SpringArm);

    bReplicates = true;

    // Audio Setup
    USceneComponent* AudioFill = CreateDefaultSubobject<USceneComponent>(TEXT("AudioFill"));
    AudioFill->SetupAttachment(BaseVehicle);

    StartEngineAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("StartEngineAudio"));
    StartEngineAudio->SetupAttachment(AudioFill);
    
    EngineAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
    EngineAudio->SetupAttachment(AudioFill);

    MovementAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("MovementAudio"));
    MovementAudio->SetupAttachment(AudioFill);
    
}

void AVehicleMaster::BeginPlay()
{
    if (NewMappingContext)
        InputMappingContext = NewMappingContext;
    
    Super::BeginPlay();

    SetReplicateMovement(true);

    if (SkeletalBaseVehicle)
        AnimInstance = Cast<UVehiclesAnimInstance>(SkeletalBaseVehicle->GetAnimInstance());

    if (SpringArm)
        SpringArm->bUsePawnControlRotation = false;

    SetupSeats();
    InitializeTurrets();
}

void AVehicleMaster::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (NewMappingContext)
        InputMappingContext = NewMappingContext;

    if (SpringArm && MainCamera)
    {
        SpringArm->bUsePawnControlRotation = false;
        MainCamera->bUsePawnControlRotation = false;
        
        SpringArm->TargetArmLength = CameraDistance;
        SpringArm->TargetOffset = FVector(0, 0, 200.f);
        
        SpringArm->bEnableCameraRotationLag = true;
    }

    if (!bCanRotateCamera)
    {
        SpringArm->bEnableCameraLag = true;
    }
}

void AVehicleMaster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AVehicleMaster, CurrentDriver);
    DOREPLIFETIME(AVehicleMaster, VehicleRoles);
    DOREPLIFETIME(AVehicleMaster, bEngineOn);
    DOREPLIFETIME(AVehicleMaster, ForwardInput);
    DOREPLIFETIME(AVehicleMaster, TurnInput);
    DOREPLIFETIME(AVehicleMaster, bCanRotateCamera);
}

void AVehicleMaster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    
    if (auto* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInput->BindAction(SwitchViewModeAction, ETriggerEvent::Started,   this, &AVehicleMaster::Input_SwitchViewMode);
        EnhancedInput->BindAction(SwitchEngine,         ETriggerEvent::Started,   this, &AVehicleMaster::Input_Client_SwitchEngine);
        EnhancedInput->BindAction(ExitAction,           ETriggerEvent::Started,   this, &AVehicleMaster::Input_OnExit);
        EnhancedInput->BindAction(MoveAction,           ETriggerEvent::Triggered, this, &AVehicleMaster::Input_OnMove);
        EnhancedInput->BindAction(MoveAction,           ETriggerEvent::Completed, this, &AVehicleMaster::Input_OnMove);
        EnhancedInput->BindAction(LookAction,           ETriggerEvent::Triggered, this, &AVehicleMaster::Input_OnUpdateCameraRotation);
    }
}


void AVehicleMaster::SetupSeats()
{
    TArray<USeatComponent*> SeatComps;
    GetComponents(SeatComps);

    SeatComps.Sort([](const USeatComponent& A, const USeatComponent& B) {
        return A.SeatIndex < B.SeatIndex;
    });

    Seats.Empty();
    for (USeatComponent* SeatComp : SeatComps)
    {
        Seats.Add(SeatComp);
    }

    SeatOccupancy.Empty();
    for (USeatComponent* SC : Seats)
    {
        FSeat Entry;
        Entry.SeatComponent      = SC;
        Entry.OccupantController = nullptr;
        Entry.OriginalPawn       = nullptr;
        
        SeatOccupancy.Add(Entry);

        if (SC->bIsDriverSeat)
        {
            SeatDriver = SC;
        }
    }
}

void AVehicleMaster::InitializeTurrets()
{
    if (!bHaveTurret)
        return;
    
    TArray<UStaticMeshComponent*> SmTurretComps;
    for (UActorComponent* Comp : GetComponentsByTag(UStaticMeshComponent::StaticClass(), TEXT("SmTurrets")))
    {
        if (auto* Mesh = Cast<UStaticMeshComponent>(Comp))
            SmTurretComps.Add(Mesh);
    }

    TArray<UTurretCamera*> CamComps;
    GetComponents<UTurretCamera>(CamComps);

    for (UTurretCamera* CamComp : CamComps)
    {
        if (!CamComp) 
            continue;
        
        ACameraVehicle* TV = CamComp->SpawnCamera(UTurretCamera::StaticClass());
        
        FName SocketName = CamComp->SkeletalSocketName;
        int SeatIdx      = CamComp->AssociateSeatIndex;
        
        CamComp->DestroyComponent();
        if (!TV) continue;

        Turrets.Add(TV);
        USceneComponent* ParentSeat = nullptr;
        UStaticMeshComponent* ParentMeshComp = nullptr;
        
        // --- Skeletal Mesh
        if (SkeletalBaseVehicle)
        {
            TV->AttachToComponent(SkeletalBaseVehicle, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);

            if (Seats.IsValidIndex(SeatIdx))
                ParentSeat = Seats[SeatIdx];
            
        }
        // --- Static Mesh
        else if (SmTurretComps.IsValidIndex(CamComps.IndexOfByKey(CamComp)))
        {
            ParentMeshComp = SmTurretComps[CamComps.IndexOfByKey(CamComp)];
            TV->AttachToComponent(ParentMeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

            if (Seats.IsValidIndex(SeatIdx))
                ParentSeat = Seats[SeatIdx];
        }
        // --- Other
        else
        {
            TV->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        }

                
        if (ParentSeat)
        {
            FRotator RelativeRot = FRotator::ZeroRotator;
            if (ParentMeshComp)
            {
                RelativeRot = ParentMeshComp->GetRelativeRotation();
            }
            else if (SkeletalBaseVehicle)
            {
                if (SocketName != NAME_None && SkeletalBaseVehicle->DoesSocketExist(SocketName))
                {
                    const FTransform SocketTransform = SkeletalBaseVehicle->GetSocketTransform(SocketName, ERelativeTransformSpace::RTS_Component);
                    RelativeRot = SocketTransform.GetRotation().Rotator();
                }
                else
                {
                    RelativeRot = TV->GetActorRotation();
                }
            }

            FTurrets NewEntry;
            NewEntry.Camera     = TV;
            NewEntry.SeatOwner  = ParentSeat;
            NewEntry.TurretMesh = ParentMeshComp;
            NewEntry.BasePitch  = RelativeRot.Pitch;
            NewEntry.BaseYaw    = RelativeRot.Yaw;
            NewEntry.LastTurretBaseRot = RelativeRot;

            SeatTurrets.Add(NewEntry);

            if (AnimInstance)
            {
                const FName Socket = TV->GetAttachParentSocketName();
                if (Socket != NAME_None)
                {
                    AnimInstance->TurretAngle.FindOrAdd(Socket) = RelativeRot;
                }
            }
        }
    }
}


void AVehicleMaster::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // --- Camera
    if (bCanRotateCamera)
    {
        FRotator VehicleRotation;
        if (BaseVehicle)
        {
            VehicleRotation = BaseVehicle->GetComponentRotation();
        }
        else if (SkeletalBaseVehicle)
        {
            VehicleRotation = SkeletalBaseVehicle->GetComponentRotation();
        }
    
        FRotator TargetSpringArmRotation = VehicleRotation + CameraRotationOffset;
        if (SpringArm)
        {
            FRotator NewRotation = FMath::RInterpTo(SpringArm->GetComponentRotation(), TargetSpringArmRotation, DeltaSeconds, 8.f);
            SpringArm->SetWorldRotation(NewRotation);
        }   
    }


    // --- Turrets
    if (HasAuthority() && !SeatTurrets.IsEmpty() && bHaveTurret)
    {
        for (FTurrets& T : SeatTurrets)
        {
            if (!T.Occupant || !T.Camera)
                continue;

            const FRotator CamRot  = T.Occupant->GetControlRotation();
            const FRotator BaseRot = T.SeatOwner->GetComponentRotation();

            const float DesiredYaw   = FMath::FindDeltaAngleDegrees(BaseRot.Yaw,   CamRot.Yaw);
            const float DesiredPitch = FMath::FindDeltaAngleDegrees(BaseRot.Pitch, CamRot.Pitch);

            const float YawError   = DesiredYaw   - T.AccumulatedYaw;
            const float PitchError = DesiredPitch - T.AccumulatedPitch;

            if (FMath::Abs(YawError) < RotationThreshold && FMath::Abs(PitchError) < RotationThreshold)
            {
                continue;
            }

            if (bUseLookAtRotation)
                ApplyLookAtRotation(TurretRotationSpeed, DeltaSeconds, T);
            else
                ApplyTurretRotation(TurretRotationSpeed, DeltaSeconds, T);
        }
    }

    // --- Sounds
    if (bEngineOn && HasAuthority())
    {
        // === Movement ===
        if (ForwardInput > 0.1f)
        {
            Multicast_StopSound(EngineAudio);
            
            if (!GetSoundIsPlaying(SoundDriveLoop, MovementAudio))
            {
                Multicast_PlaySound(SoundDriveLoop, MovementAudio);
            }
        }

        // === Idle ===
        else if (ForwardInput == 0.f)
        {
            Multicast_StopSound(MovementAudio);

            if (!GetSoundIsPlaying(SoundIdleLoop, EngineAudio))
            {
                Multicast_PlaySound(SoundIdleLoop, EngineAudio);
            }
        }
    }
}

#pragma endregion


// ------------------- Interaction -------------------
#pragma region Interfaces

// Interaction

bool AVehicleMaster::Interact_Implementation(ACustomPlayerController* PC, USceneComponent* ChosenSeat)
{
    IVehiclesInteractions::Interact_Implementation(PC, ChosenSeat);

    if (!HasAuthority() || !PC || !ChosenSeat)
        return false;
    
    if (CurrentSeats >= MaxSeat)
        return false;

    APawn* Pawn = PC->GetPawn();
    if (!Pawn)
        return false;
    
    if (GetRoleByPlayer(Pawn) != EVehiclePlaceType::None)
        return false;

    FSeat* SeatEntry = SeatOccupancy.FindByPredicate([ChosenSeat](const FSeat& S)
    {
        return S.SeatComponent == ChosenSeat;
    });
    
    if (!SeatEntry || SeatEntry->OccupantController)
        return false;

    // --- Cas Pilote ---
    if (ChosenSeat == SeatDriver)
    {
        if (GetPlayerByRole(EVehiclePlaceType::Driver))
            return false;

        CurrentDriver = PC;
        AssignRole(Pawn, EVehiclePlaceType::Driver);

        EnterVehicle(PC, ChosenSeat);
        SetOwner(PC);

        OnEnterDelegate.Broadcast();
        
        return true;
    }
    
    // --- Cas Gunner / Passenger ---
    if (!bHaveTurret)
        return false;

    AssignRole(Pawn, EVehiclePlaceType::Gunner);

    EnterVehicle(PC, ChosenSeat);
    PC->Client_AddMappingContext(NewMappingContext);

    OnEnterDelegate.Broadcast();
    
    return true;
}

void AVehicleMaster::OutOfVehicle_Implementation(ACustomPlayerController* PC)
{
    IVehiclesInteractions::OutOfVehicle_Implementation(PC);

    if (!HasAuthority()) 
        return;

    FVector ExitLocation = FindClosestExitLocation(PC);
    OnExitDelegate.Broadcast();
    
    // ----- Cas 1 : Pilote -----
    if (PC == CurrentDriver)
    {
        APawn* Pawn = ExitVehicle(PC);

        Pawn->SetActorLocation(ExitLocation);
        ReleaseRole(Pawn);
        
        CurrentDriver = nullptr;
        ForwardInput = TurnInput = 0.f;
        
        return;
    }

    // ----- Cas 2 : Gunner / Passenger -----
    for (FSeat& S : SeatOccupancy)
    {
        if (S.OccupantController == PC)
        {
            PC->Client_RemoveMappingContext(NewMappingContext);
            APawn* Pawn = ExitVehicle(PC);

            Pawn->SetActorLocation(ExitLocation);
            ReleaseRole(Pawn);
            
            return;
        }
    }
}

void AVehicleMaster::Input_OnExit()
{
  if (ACustomPlayerController* PC = Cast<ACustomPlayerController>(GetController()))
  {
      PC->ExitVehicle();
  } 
}

#pragma endregion


// ------------------- Roles Management -------------------
#pragma region Roles Management

void AVehicleMaster::AssignRole(APawn* Player, EVehiclePlaceType RoleName)
{
    if (!Player)
        return;

    bool bAlreadyAssigned = VehicleRoles.ContainsByPredicate(
        [RoleName](const FVehicleRole& R)
        {
            return R.RoleName == RoleName;
        }
    );

    if (!bAlreadyAssigned)
    {
        VehicleRoles.Add({ Player, RoleName });
    }
}

void AVehicleMaster::ReleaseRole(APawn* Player)
{
    if (!Player)
        return;

    VehicleRoles.RemoveAll(
        [Player](const FVehicleRole& R)
        {
            return R.Player == Player;
        }
    );
}

APawn* AVehicleMaster::GetPlayerByRole(EVehiclePlaceType RoleName) const
{
    const FVehicleRole* Found = VehicleRoles.FindByPredicate(
        [RoleName](const FVehicleRole& R)
        {
            return R.RoleName == RoleName;
        }
    );
    return Found ? Found->Player : nullptr;
}

EVehiclePlaceType AVehicleMaster::GetRoleByPlayer(const APawn* Player) const
{
    const FVehicleRole* Found = VehicleRoles.FindByPredicate(
        [Player](const FVehicleRole& R)
        {
            return R.Player == Player;
        }
    );
    return Found ? Found->RoleName : EVehiclePlaceType::None;
}

#pragma endregion


// ------------------- Enter/Exit -------------------
#pragma region Enter/Exit Player

void AVehicleMaster::EnterVehicle(ACustomPlayerController* PC, USceneComponent* Seat)
{
    if (!PC || !Seat) return;

    FSeat* S = SeatOccupancy.FindByPredicate( [Seat](const FSeat& E)
        { return E.SeatComponent == Seat; }
    );
    if (!S) return;

    S->OccupantController = PC;
    S->OriginalPawn = PC->GetPawn();
    ++CurrentSeats;

    ACharacter* C = Cast<ACharacter>(PC->GetPawn());
    Multicast_OnEnterSeat(C, Seat);
    
    PC->SetViewTargetWithBlend(this, 0.2f);

    for (FTurrets& T : SeatTurrets)
    {
        if (T.SeatOwner == Seat) T.Occupant = PC;
    }
}

void AVehicleMaster::Multicast_OnEnterSeat_Implementation(ACharacter* Char, USceneComponent* Seat)
{
    if (!Char || !Seat) return;
    Char->SetActorEnableCollision(false);
    Char->GetCharacterMovement()->DisableMovement();
    Char->AttachToComponent(Seat, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
 }

APawn* AVehicleMaster::ExitVehicle(ACustomPlayerController* PC)
{
    if (!PC) return nullptr;

    FSeat* Seat = SeatOccupancy.FindByPredicate(
        [PC](const FSeat& E) { return E.OccupantController == PC; }
    );
    
    if (!Seat) return nullptr;
    
    APawn* Pawn = Seat->OriginalPawn;
    if (!Pawn)
        return nullptr;
    
    if (ACharacter* CharacterToDetach = Cast<ACharacter>(Seat->OriginalPawn))
        Multicast_OnExitSeat(CharacterToDetach);

    SetOwner(nullptr);
    PC->Possess(Pawn);
    PC->SetViewTargetWithBlend(Pawn, 0.2f);

    Seat->OccupantController = nullptr;
    Seat->OriginalPawn = nullptr;

    for (FTurrets& T : SeatTurrets)
    {
        if (T.Occupant == PC) T.Occupant = nullptr;
    }
    
    --CurrentSeats;
    return Pawn;
}

void AVehicleMaster::Multicast_OnExitSeat_Implementation(ACharacter* Char)
{
    if (!Char) return;
    Char->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Char->SetActorEnableCollision(true);
    Char->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
}

FVector AVehicleMaster::FindClosestExitLocation(APlayerController* PC)
{
    
    const FSeat* S = SeatOccupancy.FindByPredicate(
        [PC](const FSeat& E){ return E.OccupantController == PC; }
    );
    
    FVector Base = S && S->SeatComponent 
        ? S->SeatComponent->GetComponentLocation()
        : GetActorLocation();
    
    UWorld* World = GetWorld();
    
    const float MinOffset = 50.f;       // Distance minimale à tester
    const float MaxOffset = 300.f;      // Distance maximale à tester
    const float StepSize = 25.f;        // Incrément de distance entre chaque test
    const float SphereRadius = 20.f;    // Rayon du trace sphérique

    float BestDistance = FLT_MAX;
    FVector BestLocation = Base;
    FVector BestDir = FVector::ZeroVector;

    TArray<FVector> Directions;
    Directions.Add(GetActorForwardVector());
    Directions.Add(-GetActorForwardVector());
    Directions.Add(GetActorRightVector());
    Directions.Add(-GetActorRightVector());
    Directions.Add((GetActorForwardVector() + GetActorRightVector()).GetSafeNormal());
    Directions.Add((GetActorForwardVector() - GetActorRightVector()).GetSafeNormal());
    Directions.Add((-GetActorForwardVector() + GetActorRightVector()).GetSafeNormal());
    Directions.Add((-GetActorForwardVector() - GetActorRightVector()).GetSafeNormal());

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(S->OriginalPawn);

    for (const FVector& Dir : Directions)
    {
        for (float Offset = MinOffset; Offset <= MaxOffset; Offset += StepSize)
        {
            FVector CandidateLocation = Base + Dir * Offset;
            
            FHitResult HitResult;
            bool bHit = World->SweepSingleByChannel(
                HitResult,
                CandidateLocation,
                CandidateLocation,
                FQuat::Identity,
                ECC_WorldStatic,
                FCollisionShape::MakeSphere(SphereRadius),
                QueryParams
            );
            
            // Affichage debug
            DrawDebugLine(World, Base, CandidateLocation, bHit ? FColor::Red : FColor::Green, false, 2.f, 0, 2.f);
            DrawDebugSphere(World, bHit ? HitResult.Location : CandidateLocation, SphereRadius, 12, bHit ? FColor::Red : FColor::Green, false, 2.f);
            
            if (!bHit)
            {
                if (Offset < BestDistance)
                {
                    BestDistance = Offset;
                    BestLocation = CandidateLocation;
                    BestDir = Dir;
                }
                break;
            }
        }
    }
    
    if (!BestDir.IsNearlyZero())
    {
        BestLocation += BestDir * OutOfVehicleOffset;
    }
    
    return BestLocation;
}

#pragma endregion


// ------------------- Camera Management -------------------
#pragma region Cameras

void AVehicleMaster::SwitchViewModeVehicle_Implementation(ACustomPlayerController* PlayerController)
{
    IVehiclesInteractions::SwitchViewModeVehicle_Implementation(PlayerController);

    Server_SwitchViewMode(PlayerController);
}

void AVehicleMaster::Input_SwitchViewMode()
{
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
        Server_SwitchViewMode(PC);
}

void AVehicleMaster::Server_SwitchViewMode_Implementation(APlayerController* PC)
{
    FSeat* S = SeatOccupancy.FindByPredicate( [PC](const FSeat& E)
    {
        return E.OccupantController == PC;
    });
    
    if (!S) return;

    Client_SwitchViewMode(PC, S->OriginalPawn);
    
}

void AVehicleMaster::Client_SwitchViewMode_Implementation(APlayerController* PC, APawn* OriginalPawn)
{
    if (PC->GetViewTarget() == this)
    {
        bCanRotateCamera = false;
        PC->SetViewTarget(OriginalPawn);
    }
    else
    {
        bCanRotateCamera = true;
        PC->SetViewTarget(this);
    }
}

void AVehicleMaster::Input_OnUpdateCameraRotation(const FInputActionValue& InputActionValue)
{
    FVector2D InputVector = InputActionValue.Get<FVector2D>();
    OnMouseMoveDelegate.Broadcast();
    
    if (bCanRotateCamera)
    {
        CameraRotationOffset.Pitch += (InputVector.Y * Sensitivity);
        CameraRotationOffset.Yaw   += (InputVector.X * Sensitivity);

        CameraRotationOffset.Pitch = FMath::Clamp(CameraRotationOffset.Pitch, -80.f, 80.f);   
    }
}

// -------- Utilities --------
ACameraVehicle* AVehicleMaster::GetAttachedCamera(FName ParentName)
{
    if (Turrets.IsEmpty())
        return nullptr;
    
    for (ACameraVehicle* Turret : Turrets)
    {
        if (Turret->GetAttachParentSocketName() == ParentName)
            return Turret;
    }
    return nullptr;
}

#pragma endregion


// ------------------- Vehicle Controls -------------------
#pragma region Vehicle Control

void AVehicleMaster::Input_Client_SwitchEngine_Implementation()
{
    Server_SwitchEngine(!bEngineOn);

    OnEngineChangeDelegate.Broadcast(!bEngineOn);
}

void AVehicleMaster::Server_SwitchEngine_Implementation(bool OnOff)
{
    if (HasAuthority())
    {
        if (OnOff)
        {
            Multicast_PlaySound(SoundEngineOn, StartEngineAudio);
        }
        else
        {
            Multicast_StopSound(MovementAudio);
            Multicast_StopSound(EngineAudio);
            
            Multicast_PlaySound(SoundEngineOff, StartEngineAudio);
            bEngineOn = OnOff;
            
            return;
        }
        
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindLambda([this, OnOff]()
        {
            bEngineOn = OnOff;
            
            if (bEngineOn)
                Multicast_PlaySound(SoundIdleLoop, EngineAudio);
        });
        
        FTimerHandle DelayTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(DelayTimerHandle, TimerDelegate, 0.5f, false); 
    }
}

void AVehicleMaster::Input_OnMove(const FInputActionValue& InputActionValue)
{
    FVector2D MovementVector = InputActionValue.Get<FVector2D>();
    Server_OnMove(MovementVector.X, MovementVector.Y);

    OnVehicleMove.Broadcast(MovementVector.X, MovementVector.Y);
}

void AVehicleMaster::Server_OnMove_Implementation(float InForward, float InTurn)
{
    ForwardInput = InForward;
    TurnInput = InTurn;

    OnVehicleMove.Broadcast(ForwardInput, TurnInput);
}

float AVehicleMaster::GetForwardInput() const
{
    return ForwardInput;
}

float AVehicleMaster::GetTurnInput() const
{
    return TurnInput;
}

#pragma endregion


// ------------------- Turret Controls -------------------
#pragma region Turret Control

void AVehicleMaster::ApplyTurretRotation(float RotationSpeed, float DeltaTime, FTurrets& TurretEntry)
{
    if (!TurretEntry.Camera || !TurretEntry.Occupant)
        return;

    const FRotator CamRot  = TurretEntry.Occupant->GetControlRotation();
    
    FName SocketName = TurretEntry.Camera->GetAttachParentSocketName();
    FRotator BaseRot = FRotator::ZeroRotator;
    
    if (SkeletalBaseVehicle)
        BaseRot = SkeletalBaseVehicle->GetSocketRotation(SocketName);
    else if (TurretEntry.TurretMesh)
        BaseRot = GetActorRotation();
    
    const float DesiredYaw   = FMath::FindDeltaAngleDegrees(BaseRot.Yaw,   CamRot.Yaw);
    const float DesiredPitch = FMath::FindDeltaAngleDegrees(BaseRot.Pitch, CamRot.Pitch);

    const float MaxDeltaThisFrame = RotationSpeed * DeltaTime;

    float YawDelta   = FMath::Clamp(DesiredYaw - TurretEntry.AccumulatedYaw, -MaxDeltaThisFrame, +MaxDeltaThisFrame);
    float PitchDelta = FMath::Clamp(DesiredPitch - TurretEntry.AccumulatedPitch, -MaxDeltaThisFrame, +MaxDeltaThisFrame);

    TurretEntry.AccumulatedYaw   = FMath::Clamp(TurretEntry.AccumulatedYaw + YawDelta, -MaxYawRotation, +MaxYawRotation);
    TurretEntry.AccumulatedPitch = FMath::Clamp(TurretEntry.AccumulatedPitch + PitchDelta, -MaxPitchRotation, +MaxPitchRotation);

    const FRotator NewLocalRot = FRotator(
        TurretEntry.BasePitch + TurretEntry.AccumulatedPitch,
         TurretEntry.BaseYaw   + TurretEntry.AccumulatedYaw,
         0.f);

    Multicast_SetTurretRotation(TurretEntry.Camera, TurretEntry.TurretMesh, NewLocalRot, FVector::ZeroVector);
}

void AVehicleMaster::ApplyLookAtRotation(float RotationSpeed, float DeltaTime, FTurrets& TurretEntry)
{
    if (!TurretEntry.Occupant || !SkeletalBaseVehicle)
        return;

    FVector CamLoc;
    FRotator CamRot;
    TurretEntry.Occupant->GetPlayerViewPoint(CamLoc, CamRot);

    FVector CamDir = CamRot.Vector();
    FVector TraceEnd = CamLoc + CamDir * 100000.f;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(TurretAim), false);
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(TurretEntry.Occupant);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_Visibility, Params);
        
    FVector DesiredLookLocation;
    if (bHit)
    {
        FVector CamToImpact = Hit.ImpactPoint - CamLoc;
        float DistOnCamRay = FVector::DotProduct(CamToImpact, CamDir);
        DesiredLookLocation = CamLoc + CamDir * DistOnCamRay;
    }
    else
    {
        DesiredLookLocation = TraceEnd;
    }

    const FName SocketName  = TurretEntry.Camera ? TurretEntry.Camera->GetAttachParentSocketName() : NAME_None;
    const FTransform BaseWS = SkeletalBaseVehicle->GetSocketTransform(SocketName, RTS_World);
    const FVector TurretWorldPos = BaseWS.GetLocation();

    TurretEntry.CurrentLookAtLocation = DesiredLookLocation;

    Multicast_SetTurretRotation(TurretEntry.Camera, TurretEntry.TurretMesh, FRotator::ZeroRotator, TurretEntry.CurrentLookAtLocation);

    DrawDebugLine(GetWorld(), CamLoc, TraceEnd, FColor::Blue, false, 0.02f, 0, 1.f);
    DrawDebugLine(GetWorld(), TurretWorldPos, DesiredLookLocation, FColor::Green, false, 0.02f, 0, 2.f);
    DrawDebugSphere(GetWorld(), DesiredLookLocation, 25.f, 12, FColor::Red, false, 0.02f);
}

void AVehicleMaster::Multicast_SetTurretRotation_Implementation(ACameraVehicle* Camera, UStaticMeshComponent* TurretMesh, FRotator TurretAngle, FVector AimLocation)
{
    if (!Camera)
        return;

    if (SkeletalBaseVehicle && AnimInstance)
    {
        FName TurretName = Camera->GetAttachParentSocketName();
        AnimInstance->UpdateTurretRotation(TurretAngle, TurretName);
        AnimInstance->UpdateTurretAimLocation(AimLocation, TurretName);
    }
    else if (BaseVehicle)
    {
        if (TurretMesh) 
        {
            TurretMesh->SetRelativeRotation(TurretAngle);
        }
    }
}

#pragma endregion


// ------------------- Sounds -------------------
#pragma region Sounds

void AVehicleMaster::Multicast_PlaySound_Implementation(USoundBase* Sound, UAudioComponent* AudioComp)
{
    if (!Sound || !AudioComp) return;
    
    AudioComp->Stop();
    AudioComp->SetSound(Sound);
    AudioComp->Play();
}

void AVehicleMaster::Multicast_StopSound_Implementation(UAudioComponent* AudioComp)
{
    if (!AudioComp) return;

    AudioComp->Stop();
}

bool AVehicleMaster::GetSoundIsPlaying(USoundBase* Sound, UAudioComponent* AudioComp)
{
    if (!Sound || !AudioComp) return false;

    return AudioComp->GetSound() == Sound && AudioComp->IsPlaying();
}

#pragma endregion

