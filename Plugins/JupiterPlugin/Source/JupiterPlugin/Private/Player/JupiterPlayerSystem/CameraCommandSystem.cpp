#include "Player/JupiterPlayerSystem/CameraCommandSystem.h"
#include "Player/PlayerCamera.h"
#include "Components/Unit/UnitOrderComponent.h"
#include "Interfaces/Selectable.h"
#include "Player/Selections/SphereRadius.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/Patrol/PatrolVisualizerComponent.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "Engine/World.h"
#include "Data/PatrolData.h"


void UCameraCommandSystem::Init(APlayerCamera* InOwner)
{
    Super::Init(InOwner);

    if (!GetOwner() || !GetWorldSafe()) return;
    
    if (GetOwner()->SphereRadiusClass)
    {
        FActorSpawnParameters Params;
        Params.Owner = GetOwner();
        SphereRadius = GetWorldSafe()->SpawnActor<ASphereRadius>(GetOwner()->SphereRadiusClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (SphereRadius)
        {
            SphereRadius->SetActorHiddenInGame(true);
        }
    }

    PatrolVisualizer = GetOwner()->GetComponentByClass<UPatrolVisualizerComponent>();
    RotationHoldThreshold = GetOwner()->RotationHoldThreshold;
    DragThreshold = GetOwner()->DragThreshold;
    LoopThreshold = GetOwner()->LoopThreshold;
}

void UCameraCommandSystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // --- A. PATROL PREVIEW OPTIMIZED ---
    if (bIsBuildingPatrolPath && PatrolVisualizer)
    {
        FHitResult Hit;
        if (GetMouseHitOnTerrain(Hit))
        {
            if (bPatrolPreviewDirty || FVector::DistSquared(Hit.Location, LastPreviewMouseLocation) > 25.0f)
            {
                UpdatePatrolPreview(Hit.Location);
                LastPreviewMouseLocation = Hit.Location;
                bPatrolPreviewDirty = false;
            }
        }
    }

    // --- B. INPUT HOLD LOGIC ---
    if (bIsRightClickDown)
    {
        FHitResult Hit;
        if (!GetMouseHitOnTerrain(Hit)) return;
        
        float TimeHeld = GetWorldSafe()->GetTimeSeconds() - ClickStartTime;
        float DistMoved = FVector::Dist2D(CommandStartLocation, Hit.Location);

        // CAS 1 : ALT + DRAG (Cercle Rayon)
        if (IsAltDown())
        {
            if (!bIsBuildingPatrolPath)
            {
                if (DistMoved > DragThreshold || CurrentMode == ECommandMode::PatrolCircle)
                {
                    CurrentMode = ECommandMode::PatrolCircle;
                    UpdateCircleRadius(Hit.Location);
                }
            }
        }
        // CAS 2 : STANDARD DRAG (Rotation Formation)
        else 
        {
            if (TimeHeld > RotationHoldThreshold || CurrentMode == ECommandMode::MoveRotation)
            {
                CurrentMode = ECommandMode::MoveRotation;
                UpdateRotationPreview(Hit.Location);

                FVector Start = CommandStartLocation;
                FVector End = Start + CurrentRotation.Vector() * 200.0f;
                DrawDebugDirectionalArrow(GetWorld(), Start, End, 20.0f, FColor::Cyan, false, -1.0f, 0, 5.0f);
            }
        }
    }
}

// ------------------------------------------------------------
// INPUT HANDLERS
// ------------------------------------------------------------

void UCameraCommandSystem::HandleAltPressed()
{
    bIsAltDown = true;
}

void UCameraCommandSystem::HandleAltReleased()
{
    bIsAltDown = false;
}

void UCameraCommandSystem::CancelPatrolCreation()
{
}

void UCameraCommandSystem::HandleCommandActionStarted()
{
    bIsRightClickDown = true;
    ClickStartTime = GetWorldSafe()->GetTimeSeconds();
    CurrentMode = ECommandMode::None; 

    FHitResult Hit;
    if (GetMouseHitOnTerrain(Hit))
    {
        CommandStartLocation = Hit.Location;
        
        if (GetOwner()->GetCameraComponent())
        {
            BaseRotation = FRotator(0.f, GetOwner()->GetCameraComponent()->GetComponentRotation().Yaw, 0.f);
        }
    	
        CurrentRotation = BaseRotation;
    }
}

void UCameraCommandSystem::HandleCommandActionCompleted()
{
    bIsRightClickDown = false;
    
    // 1. Fin du mode CERCLE
    if (CurrentMode == ECommandMode::PatrolCircle)
    {
        float Radius = (SphereRadius) ? SphereRadius->GetRadius() : 500.f;
        IssuePatrolCircleCommand(Radius, CommandStartLocation);
        
        if (SphereRadius) SphereRadius->End();
        CurrentMode = ECommandMode::None;
        return;
    }

    // 2. Fin du mode ROTATION 
    if (CurrentMode == ECommandMode::MoveRotation)
    {
        IssueMoveCommand(CommandStartLocation, CurrentRotation);
        CurrentMode = ECommandMode::None;
        return;
    }

    // 3. Click Simple -> On décide quoi faire
    FHitResult Hit;
    if (GetMouseHitOnTerrain(Hit))
    {
        ExecuteFinalCommand(Hit);
    }
    
    CurrentMode = ECommandMode::None;
}

// ------------------------------------------------------------
// STATE MACHINE & LOGIC
// ------------------------------------------------------------

void UCameraCommandSystem::ExecuteFinalCommand(const FHitResult& HitResult)
{
    // A. Gestion Patrouille (ALT KEY)
    if (IsAltDown())
    {
        if (bIsBuildingPatrolPath)
        {
            // Finir la patrouille en cours
            UE_LOG(LogTemp, Log, TEXT("Finishing Patrol Creation"));
            IssuePatrolPathCommand(); 
            ResetPatrolPath();        
        }
        else
        {
            // Commencer une nouvelle patrouille
            UE_LOG(LogTemp, Log, TEXT("Starting Patrol Creation"));
            bIsBuildingPatrolPath = true;
            PatrolWaypoints.Empty();
            AddPatrolWaypoint(HitResult.Location);
        }
        return;
    }

    // B. Ajout de point (Si mode construction actif)
    if (bIsBuildingPatrolPath)
    {
        AddPatrolWaypoint(HitResult.Location);
        return;
    }

    // C. Commandes Standard (Attaque ou Mouvement)
    AActor* Target = GetHoveredActor();
    
    if (Target && Target->Implements<USelectable>() && Target != GetOwner())
    {
        IssueAttackCommand(Target);
    }
    else
    {
        IssueMoveCommand(HitResult.Location, BaseRotation);
    }
}

// ------------------------------------------------------------
// ISSUE COMMANDS
// ------------------------------------------------------------

void UCameraCommandSystem::IssueMoveCommand(const FVector& TargetLocation, const FRotator& Facing)
{
    if (auto* OrderComp = GetOwner()->GetOrderComponent())
    {
        FCommandData Data(GetOwner()->GetPlayerController(), TargetLocation, Facing, ECommandType::CommandMove);
        OrderComp->IssueOrder(Data);
    }
}

void UCameraCommandSystem::IssueAttackCommand(AActor* TargetActor)
{
    if (auto* OrderComp = GetOwner()->GetOrderComponent())
    {
        FCommandData Data(GetOwner()->GetPlayerController(), FVector::ZeroVector, FRotator::ZeroRotator, ECommandType::CommandAttack, TargetActor);
        OrderComp->IssueOrder(Data);
    }
}

void UCameraCommandSystem::IssuePatrolCircleCommand(float Radius, const FVector& Center)
{
    if (auto* OrderComp = GetOwner()->GetOrderComponent())
    {
        FCommandData Data(GetOwner()->GetPlayerController(), Center, FRotator::ZeroRotator, ECommandType::CommandPatrol);
        Data.Radius = Radius;
        OrderComp->IssueOrder(Data);
    }
}

void UCameraCommandSystem::IssuePatrolPathCommand()
{
    if (!GetOwner()->GetOrderComponent())
    	return;

    if (PatrolWaypoints.Num() < 2) 
    {
        UE_LOG(LogTemp, Warning, TEXT("Patrol path too short (Need 2+ points)."));
        return;
    }

    FCommandData Data(GetOwner()->GetPlayerController(), PatrolWaypoints[0], FRotator::ZeroRotator, ECommandType::CommandPatrol);
    
    const float DistFirstToLast = FVector::Dist(PatrolWaypoints[0], PatrolWaypoints.Last());
    const bool bIsLoop = (DistFirstToLast < LoopThreshold);
    
    Data.bPatrolLoop = bIsLoop;
    
    if (bIsLoop)
    {
        Data.PatrolPath.Reserve(PatrolWaypoints.Num() - 1);
        for (int i = 0; i < PatrolWaypoints.Num() - 1; i++)
        {
            Data.PatrolPath.Add(PatrolWaypoints[i]);
        }
    }
    else
    {
        Data.PatrolPath = PatrolWaypoints;
    }

    GetOwner()->GetOrderComponent()->IssueOrder(Data);
}

// ------------------------------------------------------------
// PREVIEWS & VISUALS
// ------------------------------------------------------------

void UCameraCommandSystem::UpdateCircleRadius(const FVector& MouseLocation)
{
    if (SphereRadius)
    {
        SphereRadius->Start(CommandStartLocation, FRotator::ZeroRotator);
        SphereRadius->SetRadius(FVector::Dist2D(CommandStartLocation, MouseLocation));
    }
}

void UCameraCommandSystem::UpdateRotationPreview(const FVector& MouseLocation)
{
    FVector Dir = MouseLocation - CommandStartLocation;
    Dir.Z = 0.f;

    if (!Dir.IsNearlyZero())
    {
        Dir.Normalize();
        CurrentRotation = FRotationMatrix::MakeFromX(Dir).Rotator();
    }
}

void UCameraCommandSystem::AddPatrolWaypoint(const FVector& Location)
{
    if (PatrolWaypoints.Num() > 0 && FVector::DistSquared(PatrolWaypoints.Last(), Location) < 10000.0f)
    {
        return;
    }
    
    PatrolWaypoints.Add(Location);
    bPatrolPreviewDirty = true;
}

void UCameraCommandSystem::ResetPatrolPath()
{
    bIsBuildingPatrolPath = false;
    PatrolWaypoints.Empty();
    ClearPatrolPreview();
}

void UCameraCommandSystem::UpdatePatrolPreview(const FVector& CurrentMouseLocation)
{
    if (!PatrolVisualizer || PatrolWaypoints.Num() == 0)
    	return;
    
    TArray<FVector> DisplayPoints = PatrolWaypoints;
    DisplayPoints.Add(CurrentMouseLocation);
    
    FPatrolRouteExtended PreviewRoute;
    PreviewRoute.PatrolPoints = DisplayPoints;
    PreviewRoute.PatrolType = EPatrolType::Once;
    PreviewRoute.RouteColor = FLinearColor::Yellow;
    PreviewRoute.bShowWaypointNumbers = true;
    
    TArray<FPatrolRouteExtended> Routes;
    Routes.Add(PreviewRoute);
    
    PatrolVisualizer->UpdateVisualization(Routes);
}

void UCameraCommandSystem::ClearPatrolPreview()
{
    if (PatrolVisualizer)
    {
        PatrolVisualizer->UpdateVisualization({});
    }
}

// ------------------------------------------------------------
// DESTRUCTION LOGIC (Networked)
// ------------------------------------------------------------

void UCameraCommandSystem::HandleDestroySelected()
{
    if (auto* SelComp = GetOwner()->GetSelectionComponent())
    {
        HandleServerDestroyActor(SelComp->GetSelectedActors());
    }
}

void UCameraCommandSystem::HandleServerDestroyActor(const TArray<AActor*>& ActorsToDestroy)
{
    if (ActorsToDestroy.Num() == 0)
    	return;
	
    // TODO: Implémenter un RPC dans Controller ou UnitOrderComponent
    
    if (GetOwner()->HasAuthority())
    {
        for (AActor* A : ActorsToDestroy)
        {
            if (IsValid(A))
            	A->Destroy();
        }
    }
    else
    {
        // RCP Destroy
    }
}

// ------------------------------------------------------------
// UTILS
// ------------------------------------------------------------

bool UCameraCommandSystem::GetMouseHitOnTerrain(FHitResult& OutHit) const
{
    if (auto* SelComp = GetOwner()->GetSelectionComponent())
    {
        OutHit = SelComp->GetMousePositionOnTerrain();
        return OutHit.bBlockingHit;
    }
	
    return false;
}

AActor* UCameraCommandSystem::GetHoveredActor() const
{
    if (!GetOwner() || !GetWorldSafe())
    	return nullptr;

    APlayerController* PC = GetOwner()->GetPlayerController();
    if (!PC)
    	return nullptr;

    FVector WorldLoc, WorldDir;
    if (PC->DeprojectMousePositionToWorld(WorldLoc, WorldDir))
    {
        FVector End = WorldLoc + WorldDir * 100000.f;
        FHitResult Hit;
        
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(GetOwner());
        Params.bTraceComplex = true; 

        if (GetWorldSafe()->LineTraceSingleByChannel(Hit, WorldLoc, End, ECC_Visibility, Params))
        {
            return Hit.GetActor();
        }
    }
    
    return nullptr;
}