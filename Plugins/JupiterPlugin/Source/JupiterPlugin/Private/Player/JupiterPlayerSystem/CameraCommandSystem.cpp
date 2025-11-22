#include "Player/JupiterPlayerSystem/CameraCommandSystem.h"
#include "Player/PlayerCamera.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Interfaces/Selectable.h"
#include "Player/Selections/SphereRadius.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"

// ------------------------------------------------------------
// INIT
// ------------------------------------------------------------
void UCameraCommandSystem::Init(APlayerCamera* InOwner)
{
    Super::Init(InOwner);

    if (GetWorldSafe() && SphereRadiusClass)
    {
        FActorSpawnParameters Params;
        Params.Owner = GetOwner();
        
        SphereRadius = GetWorldSafe()->SpawnActor<ASphereRadius>(SphereRadiusClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (SphereRadius)
        {
            SphereRadius->SetActorHiddenInGame(true);
        }
    }
}

// ------------------------------------------------------------
// TICK : Gestion des États + DEBUG VISUEL
// ------------------------------------------------------------
void UCameraCommandSystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // --- A. VISUALISATION DEBUG (Patrol Path) ---
    if (bIsBuildingPatrolPath)
    {
        // 1. Dessiner les lignes entre les points existants
        if (PatrolWaypoints.Num() > 0)
        {
            for (int i = 0; i < PatrolWaypoints.Num() - 1; i++)
            {
                DrawDebugLine(GetWorld(), PatrolWaypoints[i], PatrolWaypoints[i+1], FColor::Green, false, -1.0f, 0, 3.0f);
                DrawDebugSphere(GetWorld(), PatrolWaypoints[i], 15.0f, 8, FColor::Green, false, -1.0f, 0, 1.0f);
            }
            // Marquer le dernier point validé
            DrawDebugSphere(GetWorld(), PatrolWaypoints.Last(), 15.0f, 8, FColor::Green, false, -1.0f, 0, 1.0f);
        }

        // 2. Dessiner la ligne élastique vers la souris (Preview du prochain point)
        FHitResult Hit;
        if(GetMouseHitOnTerrain(Hit))
        {
            FVector StartPos = (PatrolWaypoints.Num() > 0) ? PatrolWaypoints.Last() : CommandStartLocation;
            DrawDebugLine(GetWorld(), StartPos, Hit.Location, FColor::Yellow, false, -1.0f, 0, 2.0f);
            DrawDebugSphere(GetWorld(), Hit.Location, 10.0f, 8, FColor::Yellow, false, -1.0f, 0, 1.0f);
            
            // Afficher le status texte à côté de la souris pour le debug
            FString StatusText = FString::Printf(TEXT("PATROL BUILDER\nPoints: %d\n[R-Click] Add Point\n[Alt+R-Click] Finish"), PatrolWaypoints.Num());
            DrawDebugString(GetWorld(), Hit.Location + FVector(0,0,100), StatusText, nullptr, FColor::White, 0.0f, true);
        }
    }

    // --- B. LOGIQUE INPUT MAINTENU ---
    if (bIsRightClickDown)
    {
        FHitResult Hit;
        if (!GetMouseHitOnTerrain(Hit)) return;
        
        float TimeHeld = GetWorldSafe()->GetTimeSeconds() - ClickStartTime;
        float DistMoved = FVector::Dist2D(CommandStartLocation, Hit.Location);

        // CAS 1 : ALT + CLIC MAINTENU
        if (IsAltDown())
        {
            // Si on est déjà en train de construire un chemin point par point, 
            // on INTERDIT de passer en mode "Cercle" (Slide) pour éviter de casser la logique.
            if (!bIsBuildingPatrolPath)
            {
                // Si on bouge la souris significativement -> Mode Cercle
                if (DistMoved > DragThreshold || CurrentMode == ECommandMode::PatrolCircle)
                {
                    CurrentMode = ECommandMode::PatrolCircle;
                    UpdateCircleRadius(Hit.Location);
                }
            }
        }
        // CAS 2 : CLIC MAINTENU (SANS ALT) -> ROTATION PREVIEW
        else 
        {
            // Si on maintient assez longtemps -> Mode Rotation
            if (TimeHeld > RotationHoldThreshold || CurrentMode == ECommandMode::MoveRotation)
            {
                CurrentMode = ECommandMode::MoveRotation;
                UpdateRotationPreview(Hit.Location);

                // VISUALISATION DEBUG (Rotation Arrow)
                FVector Start = CommandStartLocation;
                FVector End = Start + CurrentRotation.Vector() * 200.0f; // 200 units long arrow
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
        
        // Reset rotation de base
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
    
    // 1. Fin du mode CERCLE (Alt + Drag)
    if (CurrentMode == ECommandMode::PatrolCircle)
    {
        float Radius = (SphereRadius) ? SphereRadius->GetRadius() : 500.f;
        IssuePatrolCircleCommand(Radius, CommandStartLocation);
        
        if (SphereRadius) SphereRadius->End();
        CurrentMode = ECommandMode::None;
        return;
    }

    // 2. Fin du mode ROTATION (Hold Right Click)
    if (CurrentMode == ECommandMode::MoveRotation)
    {
        IssueMoveCommand(CommandStartLocation, CurrentRotation);
        CurrentMode = ECommandMode::None;
        return;
    }

    // 3. Sinon, c'était un CLIC RAPIDE -> On délègue à ExecuteFinalCommand
    FHitResult Hit;
    if (GetMouseHitOnTerrain(Hit))
    {
        ExecuteFinalCommand(Hit);
    }
    
    CurrentMode = ECommandMode::None;
}

// ------------------------------------------------------------
// LOGIC CORE : GESTION DES ETATS DE PATROUILLE
// ------------------------------------------------------------

void UCameraCommandSystem::ExecuteFinalCommand(const FHitResult& HitResult)
{
    // Debug Log pour suivre l'action
    UE_LOG(LogTemp, Log, TEXT("ExecuteFinalCommand - Alt: %d, BuildingPatrol: %d"), IsAltDown(), bIsBuildingPatrolPath);

    // --- MODE PATROUILLE PAR POINTS ---
    
    // Cas A : Alt + Clic (Rapide) -> Soit on COMMENCE, soit on FINIT
    if (IsAltDown())
    {
        if (!bIsBuildingPatrolPath)
        {
            // START : On commence une nouvelle patrouille
            UE_LOG(LogTemp, Warning, TEXT(">>> START PATROL CREATION <<<"));
            
            bIsBuildingPatrolPath = true;
            PatrolWaypoints.Empty();
            AddPatrolWaypoint(HitResult.Location);
        }
        else
        {
            // END : On termine la patrouille
            UE_LOG(LogTemp, Warning, TEXT(">>> FINISH PATROL CREATION <<<"));
            
            // On ajoute le point final (là où on a cliqué pour finir)
            AddPatrolWaypoint(HitResult.Location);
            
            // On envoie l'ordre aux unités
            IssuePatrolPathCommand(); 
            
            // On nettoie
            ResetPatrolPath();        
        }
        return;
    }

    // Cas B : Clic Droit (Rapide, SANS Alt) -> Soit on AJOUTE UN POINT, soit on BOUGE
    if (bIsBuildingPatrolPath)
    {
        // On est en mode construction, le clic droit sert à ajouter des points intermédiaires
        UE_LOG(LogTemp, Log, TEXT(">>> ADD WAYPOINT (%d) <<<"), PatrolWaypoints.Num() + 1);
        AddPatrolWaypoint(HitResult.Location);
        return;
    }

    // Cas C : Clic Droit Standard (Move ou Attack normal)
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
// ISSUE COMMANDS (Envoi des ordres)
// ------------------------------------------------------------

void UCameraCommandSystem::IssueMoveCommand(const FVector& TargetLocation, const FRotator& Facing)
{
    if (!GetOrderComponent()) return;

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
    if (!GetOrderComponent()) return;

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
    if (!GetOrderComponent()) return;

    FCommandData Data(
        GetOwner()->GetPlayerController(),
        Center,
        FRotator::ZeroRotator,
        ECommandType::CommandPatrol
    );

    Data.Radius = Radius;
    // Optionnel si ton enum le supporte : Data.PatrolType = EPatrolType::Circle; 

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

    // Utilise le premier point comme destination immédiate
    FCommandData Data(
        GetOwner()->GetPlayerController(),
        PatrolWaypoints[0],
        FRotator::ZeroRotator,
        ECommandType::CommandPatrol
    );

    // --- CORRECTION CRITIQUE ---
    // On remplit le tableau de points pour que UnitPatrolComponent puisse le lire
    Data.PatrolPath = PatrolWaypoints; 
    Data.bPatrolLoop = true; // Par défaut, on boucle
    // ---------------------------

    UE_LOG(LogTemp, Warning, TEXT("Issuing Patrol Order with %d waypoints."), Data.PatrolPath.Num());

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
    // Évite d'ajouter des points trop proches (double clic accidentel)
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
}

void UCameraCommandSystem::HandleDestroySelected()
{
    if (!GetSelectionComponent()) return;
    HandleServerDestroyActor(GetSelectionComponent()->GetSelectedActors());
}

void UCameraCommandSystem::HandleServerDestroyActor(const TArray<AActor*>& ActorsToDestroy)
{
    for (AActor* A : ActorsToDestroy)
    {
        if (A) A->Destroy();
    }
}

bool UCameraCommandSystem::GetMouseHitOnTerrain(FHitResult& OutHit) const
{
    if (!GetSelectionComponent()) return false;
    OutHit = GetSelectionComponent()->GetMousePositionOnTerrain();
    return OutHit.bBlockingHit;
}

AActor* UCameraCommandSystem::GetHoveredActor() const
{
    if (!GetOwner() || !GetWorldSafe()) return nullptr;

    APlayerController* PC = GetOwner()->GetPlayerController();
    if (!PC) return nullptr;

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

bool UCameraCommandSystem::IsAltDown() const
{
    if (bIsAltDown) return true;

    if (GetOwner() && GetOwner()->GetPlayerController())
    {
        return GetOwner()->GetPlayerController()->IsInputKeyDown(EKeys::LeftAlt) || 
               GetOwner()->GetPlayerController()->IsInputKeyDown(EKeys::RightAlt);
    }

    return false;
}