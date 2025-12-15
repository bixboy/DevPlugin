#include "Player/JupiterPlayerSystem/CameraCommandSystem.h"
#include "Player/PlayerCamera.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Interfaces/Selectable.h"
#include "Player/Selections/SphereRadius.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Components/PatrolVisualizerComponent.h"
#include "Data/PatrolData.h"


void UCameraCommandSystem::Init(APlayerCamera* InOwner)
{
    Super::Init(InOwner);

	if (!GetOwner() || !GetWorldSafe())
		return;
	
    FActorSpawnParameters Params;
    Params.Owner = GetOwner();
    
    SphereRadius = GetWorldSafe()->SpawnActor<ASphereRadius>(GetOwner()->SphereRadiusClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
    if (SphereRadius)
    {
        SphereRadius->SetActorHiddenInGame(true);
    }

	
	PatrolVisualizer = GetOwner()->GetComponentByClass<UPatrolVisualizerComponent>();
	RotationHoldThreshold = GetOwner()->RotationHoldThreshold;
	DragThreshold = GetOwner()->DragThreshold;
	LoopThreshold = GetOwner()->LoopThreshold;
}


void UCameraCommandSystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // --- A. UPDATE PATROL PREVIEW ---
    if (bIsBuildingPatrolPath)
    {
        UpdatePatrolPreview();
    }

    // --- B. LOGIQUE INPUT MAINTENU ---
    if (bIsRightClickDown)
    {
        FHitResult Hit;
        if (!GetMouseHitOnTerrain(Hit))
         return;
        
        float TimeHeld = GetWorldSafe()->GetTimeSeconds() - ClickStartTime;
        float DistMoved = FVector::Dist2D(CommandStartLocation, Hit.Location);

        // CAS 1 : ALT + CLIC MAINTENU
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
        // CAS 2 : ROTATION PREVIEW
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
    UE_LOG(LogTemp, Log, TEXT("HandleAltPressed"));
    bIsAltDown = true;
}

void UCameraCommandSystem::HandleAltReleased()
{
    UE_LOG(LogTemp, Log, TEXT("HandleAltReleased"));
    bIsAltDown = false;
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
        
        if (SphereRadius)
        	SphereRadius->End();
    	
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

    // 3. On délègue à ExecuteFinalCommand
    FHitResult Hit;
    if (GetMouseHitOnTerrain(Hit))
    {
        ExecuteFinalCommand(Hit);
    }
    
    CurrentMode = ECommandMode::None;
}

// ------------------------------------------------------------
// GESTION DES ETATS DE PATROUILLE
// ------------------------------------------------------------

void UCameraCommandSystem::ExecuteFinalCommand(const FHitResult& HitResult)
{
    UE_LOG(LogTemp, Log, TEXT("ExecuteFinalCommand - Alt: %d, BuildingPatrol: %d"), IsAltDown(), bIsBuildingPatrolPath);
	
    // Cas A : Soit on COMMENCE, soit on FINIT
    if (IsAltDown())
    {
        if (!bIsBuildingPatrolPath)
        {
            UE_LOG(LogTemp, Warning, TEXT(">>> START PATROL CREATION <<<"));
            
            bIsBuildingPatrolPath = true;
            PatrolWaypoints.Empty();
            AddPatrolWaypoint(HitResult.Location);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT(">>> FINISH PATROL CREATION <<<"));
            
            IssuePatrolPathCommand(); 
            ResetPatrolPath();        
        }
        return;
    }

    // Cas B : Soit on AJOUTE UN POINT, soit on BOUGE
    if (bIsBuildingPatrolPath)
    {
        UE_LOG(LogTemp, Log, TEXT(">>> ADD WAYPOINT (%d) <<<"), PatrolWaypoints.Num() + 1);
    	
        AddPatrolWaypoint(HitResult.Location);
        return;
    }

    // Cas C : Move ou Attack normal
    AActor* Target = GetHoveredActor();
    if (Target && Target->Implements<USelectable>())
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
    if (!GetOrderComponent())
     return;

    FCommandData Data(
        GetOwner()->GetPlayerController(),
        TargetLocation,
        Facing,
        ECommandType::CommandMove
    );
    
    GetOrderComponent()->IssueOrder(Data);
}

void UCameraCommandSystem::IssueAttackCommand(AActor* TargetActor)
{
    if (!GetOrderComponent()) 
    return;

    FCommandData Data(
        GetOwner()->GetPlayerController(),
        FVector::ZeroVector, 
        FRotator::ZeroRotator,
        ECommandType::CommandAttack,
        TargetActor
    );

    GetOrderComponent()->IssueOrder(Data);
}

void UCameraCommandSystem::IssuePatrolCircleCommand(float Radius, const FVector& Center)
{
    if (!GetOrderComponent()) 
    return;

    FCommandData Data(
        GetOwner()->GetPlayerController(),
        Center,
        FRotator::ZeroRotator,
        ECommandType::CommandPatrol
    );

    Data.Radius = Radius;
    GetOrderComponent()->IssueOrder(Data);
}

void UCameraCommandSystem::IssuePatrolPathCommand()
{
    if (!GetOrderComponent()) 
    {
        UE_LOG(LogTemp, Error, TEXT("IssuePatrolPathCommand: No Order Component!"));
        return;
    }

    if (PatrolWaypoints.Num() < 2) 
    {
        UE_LOG(LogTemp, Warning, TEXT("IssuePatrolPathCommand: Ignored (Only %d points). Need at least 2."), PatrolWaypoints.Num());
        return;
    }

    FCommandData Data(
        GetOwner()->GetPlayerController(),
        PatrolWaypoints[0],
        FRotator::ZeroRotator,
        ECommandType::CommandPatrol
    );
	
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
    // ---------------------------

    UE_LOG(LogTemp, Warning, TEXT("Issuing Patrol Order with %d waypoints. Loop: %s (Dist to start: %.1f)"), 
        Data.PatrolPath.Num(), 
        Data.bPatrolLoop ? TEXT("YES") : TEXT("NO"),
        DistFirstToLast);

    GetOrderComponent()->IssueOrder(Data);
}

// ------------------------------------------------------------
// HELPERS
// ------------------------------------------------------------

void UCameraCommandSystem::UpdateCircleRadius(const FVector& MouseLocation)
{
    if (SphereRadius)
    {
        SphereRadius->Start(CommandStartLocation, FRotator::ZeroRotator);
        float Dist = FVector::Dist2D(CommandStartLocation, MouseLocation);
        SphereRadius->SetRadius(Dist);
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
    if (PatrolWaypoints.Num() > 0 && FVector::DistSquared(PatrolWaypoints.Last(), Location) < 100.0f)
    {
        return;
    }
	
    PatrolWaypoints.Add(Location);
}

void UCameraCommandSystem::ResetPatrolPath()
{
    bIsBuildingPatrolPath = false;
    PatrolWaypoints.Empty();
    ClearPatrolPreview();
}

void UCameraCommandSystem::CancelPatrolCreation()
{
    if (bIsBuildingPatrolPath)
    {
        UE_LOG(LogTemp, Log, TEXT(">>> PATROL CREATION CANCELLED <<<"));
        ResetPatrolPath();
    }
}

void UCameraCommandSystem::UpdatePatrolPreview()
{
    if (!PatrolVisualizer || PatrolWaypoints.Num() == 0)
        return;
    
    FPatrolRouteExtended PreviewRoute;
    PreviewRoute.PatrolPoints = PatrolWaypoints;
    PreviewRoute.PatrolType = EPatrolType::Once;
    PreviewRoute.RouteColor = FLinearColor(1.0f, 0.8f, 0.0f);
    PreviewRoute.bShowWaypointNumbers = true;
    PreviewRoute.bShowDirectionArrows = false;
    
    TArray<FPatrolRouteExtended> Routes;
    Routes.Add(PreviewRoute);
    
    PatrolVisualizer->UpdateVisualization(Routes);
}

void UCameraCommandSystem::ClearPatrolPreview()
{
    if (!PatrolVisualizer)
        return;
    
    TArray<FPatrolRouteExtended> EmptyRoutes;
    PatrolVisualizer->UpdateVisualization(EmptyRoutes);
}

void UCameraCommandSystem::HandleDestroySelected()
{
    if (!GetSelectionComponent())
    	return;
	
    HandleServerDestroyActor(GetSelectionComponent()->GetSelectedActors());
}

void UCameraCommandSystem::HandleServerDestroyActor(const TArray<AActor*>& ActorsToDestroy)
{
    for (AActor* A : ActorsToDestroy)
    {
        if (A)
        	A->Destroy();
    }
}

bool UCameraCommandSystem::GetMouseHitOnTerrain(FHitResult& OutHit) const
{
    if (!GetSelectionComponent())
    	return false;
	
    OutHit = GetSelectionComponent()->GetMousePositionOnTerrain();
    return OutHit.bBlockingHit;
}

AActor* UCameraCommandSystem::GetHoveredActor() const
{
    if (!GetOwner() || !GetWorldSafe())
    	return nullptr;

    APlayerController* PC = GetOwner()->GetPlayerController();
    if (!PC)
    	return nullptr;

    FVector WL, WD;
    if (PC->DeprojectMousePositionToWorld(WL, WD))
    {
        FVector End = WL + WD * 100000.f;
        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(GetOwner());

        if (GetWorldSafe()->LineTraceSingleByChannel(Hit, WL, End, ECC_Visibility, Params))
        {
            return Hit.GetActor();
        }
    }
	
    return nullptr;
}