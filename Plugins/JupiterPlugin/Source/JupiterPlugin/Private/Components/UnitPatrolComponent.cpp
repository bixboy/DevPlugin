#include "Components/UnitPatrolComponent.h"
#include "Components/PatrolVisualizerComponent.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Algo/Reverse.h"
#include "Algo/Rotate.h"
#include "Components/CommandComponent.h"
#include "Interfaces/Selectable.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Units/AI/AiControllerRts.h"


UUnitPatrolComponent::UUnitPatrolComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
	SetIsReplicatedByDefault(true);
}

void UUnitPatrolComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
		return;

	SelectionComponent = OwnerActor->FindComponentByClass<UUnitSelectionComponent>();
	OrderComponent = OwnerActor->FindComponentByClass<UUnitOrderComponent>();

	if (OrderComponent)
	{
		OrderComponent->OnOrdersDispatched.AddDynamic(this, &UUnitPatrolComponent::HandleOrdersDispatched);
	}

	if (SelectionComponent)
	{
		SelectionComponent->OnSelectionChanged.AddDynamic(this, &UUnitPatrolComponent::OnSelectionChanged);
	}

	if (IsLocallyControlled() && bAutoCreateVisualizer)
	{
		EnsureVisualizerComponent();
	}
}

void UUnitPatrolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UUnitPatrolComponent, ActivePatrolRoutes);
}

bool UUnitPatrolComponent::IsLocallyControlled() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
		return false;

	APawn* Pawn = Cast<APawn>(Owner);
	if (Pawn && Pawn->IsLocallyControlled())
		return true;

	return Owner->GetLocalRole() == ROLE_Authority;
}

bool UUnitPatrolComponent::HasAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

void UUnitPatrolComponent::RefreshRoutesFromSelection()
{
	if (!SelectionComponent || !IsLocallyControlled())
		return;

	TArray<AActor*> SelectedUnits = SelectionComponent->GetSelectedActors();
	TArray<FPatrolRoute> NewRoutes;

	for (AActor* Unit : SelectedUnits)
	{
		if (!Unit || !Unit->Implements<USelectable>())
			continue;

		FPatrolRoute Route;
		if (GetPatrolRouteForUnit(Unit, Route))
		{
			bool bFound = false;
			for (FPatrolRoute& ExistingRoute : NewRoutes)
			{
				if (ExistingRoute.PatrolID.IsValid() && ExistingRoute.PatrolID == Route.PatrolID)
				{
					ExistingRoute.AssignedUnits.AddUnique(Unit);
					bFound = true;
					UE_LOG(LogTemp, Verbose, TEXT("[UnitPatrolComponent] Unit %s added to existing patrol %s"), *Unit->GetName(), *Route.PatrolID.ToString());

					break;
				}
			}

			if (!bFound)
			{
				Route.AssignedUnits.Add(Unit);
				NewRoutes.Add(Route);
				
				UE_LOG(LogTemp, Verbose, TEXT("[UnitPatrolComponent] New patrol route added with ID %s"), *Route.PatrolID.ToString());
			}
		}
	}

	ApplyRoutes(NewRoutes);
}

// ... (HandleOrdersDispatched)
void UUnitPatrolComponent::HandleOrdersDispatched(const TArray<AActor*>& AffectedUnits, const FCommandData& IssuedCommand)
{
	if (IssuedCommand.Type == CommandPatrol && IssuedCommand.PatrolPath.Num() >= 2)
	{
		// ... Existing creation logic ...
		FPatrolCreationParams Params;
		Params.Points = IssuedCommand.PatrolPath;
		Params.Units = AffectedUnits;
		Params.Type = IssuedCommand.bPatrolLoop ? EPatrolType::Loop : EPatrolType::Once;
		
		for (AActor* Unit : AffectedUnits)
		{
			if (const FPatrolRoute* Cached = UnitRouteCache.Find(Unit))
			{
				Params.Name = Cached->RouteName;
				Params.Color = Cached->RouteColor;
				Params.bShowArrows = Cached->bShowArrows;
				Params.bShowNumbers = Cached->bShowNumbers;
				break; 
			}
		}
		if (Params.Color == FLinearColor::Blue) Params.Color = RouteColor;

		if (HasAuthority())
		{
			Server_CreatePatrol_Implementation(Params);
		}
		else
		{
			Server_CreatePatrol(Params);
		}
	}
	else
	{
		// If command is NOT a patrol command (e.g. Move, Attack, Stop), 
		// units must be removed from their current patrols.
		for (AActor* Unit : AffectedUnits)
		{
			if (HasAuthority())
			{
				Server_RemovePatrolRouteForUnit_Implementation(Unit);
			}
			else
			{
				Server_RemovePatrolRouteForUnit(Unit);
			}
		}
	}
}

// ... 

void UUnitPatrolComponent::OnSelectionChanged(const TArray<AActor*>& SelectedActors)
{
	RefreshRoutesFromSelection();
}



void UUnitPatrolComponent::ApplyRoutes(const TArray<FPatrolRoute>& NewRoutes)
{
	ActivePatrolRoutes = NewRoutes;
	
	if (IsLocallyControlled())
	{
		UpdateVisualization();
		OnPatrolRoutesChanged.Broadcast();
	}
}

void UUnitPatrolComponent::OnRep_ActivePatrolRoutes()
{
	UnitRouteCache.Empty();
	for (const FPatrolRoute& Route : ActivePatrolRoutes)
	{
		for (TObjectPtr UnitPtr : Route.AssignedUnits)
		{
			if (AActor* Unit = UnitPtr.Get())
			{
				UnitRouteCache.Add(Unit, Route);
			}
		}
	}

	if (IsLocallyControlled())
	{
		UpdateVisualization();
		OnPatrolRoutesChanged.Broadcast();
	}
}

void UUnitPatrolComponent::Server_UpdatePatrolRoute_Implementation(int32 Index, const FPatrolRoute& NewRoute)
{
	if (ActivePatrolRoutes.IsValidIndex(Index))
	{
		ActivePatrolRoutes[Index] = NewRoute;
		
		UE_LOG(LogTemp, Log, TEXT("[UnitPatrolComponent] Updating patrol route %d (ID: %s) for %d units"), 
			Index, *NewRoute.PatrolID.ToString(), NewRoute.AssignedUnits.Num());
		
		for (TObjectPtr UnitPtr : NewRoute.AssignedUnits)
		{
			if (AActor* Unit = UnitPtr.Get())
			{
				UpdatePatrolOrderForUnit(Unit, NewRoute, false);
			}
		}

		if (IsLocallyControlled())
		{
			UpdateVisualization();
		}
	}
}

void UUnitPatrolComponent::Server_UpdateUnitPatrol_Implementation(AActor* Unit, const FPatrolRoute& NewRoute)
{
	if (!Unit)
		return;

	// Update Cache
	UnitRouteCache.Add(Unit, NewRoute);

	// Issue Order to update Unit's behavior
	// Note: Ideally we would issue a new command here, but for now we rely on the cache 
	// and the fact that the unit might already have the command or will get it.
	// If we want to support live geometry updates, we'd need to re-issue the order.
	
	RefreshRoutesFromSelection();
}

void UUnitPatrolComponent::Server_RemovePatrolRoute_Implementation(int32 Index)
{
	if (ActivePatrolRoutes.IsValidIndex(Index))
	{
		const FPatrolRoute& RouteToRemove = ActivePatrolRoutes[Index];
		TArray<AActor*> UnitsToStop;
		
		for (const auto& Ptr : RouteToRemove.AssignedUnits)
		{
			if (AActor* Unit = Ptr.Get())
			{
				UnitsToStop.Add(Unit);
			}
		}

		FGuid RemovedID = RouteToRemove.PatrolID;

		// 1. Stop Units
		StopUnits(UnitsToStop);

		// 2. Remove from Active List
		ActivePatrolRoutes.RemoveAt(Index);
		
		if (IsLocallyControlled())
		{
			UpdateVisualization();
		}

		// 3. Multicast to clear Caches and Sync Clients immediately
		Multicast_RemovePatrolRoute(UnitsToStop, RemovedID);
	}
}

void UUnitPatrolComponent::Multicast_RemovePatrolRoute_Implementation(const TArray<AActor*>& Units, const FGuid& PatrolID)
{
	for (int32 i = ActivePatrolRoutes.Num() - 1; i >= 0; --i)
	{
		if (ActivePatrolRoutes[i].PatrolID == PatrolID)
		{
			ActivePatrolRoutes.RemoveAt(i);
		}
	}

	UnitRouteCache.Empty();
	for (const FPatrolRoute& Route : ActivePatrolRoutes)
	{
		for (TObjectPtr UnitPtr : Route.AssignedUnits)
		{
			if (AActor* Unit = UnitPtr.Get())
			{
				UnitRouteCache.Add(Unit, Route);
			}
		}
	}

	if (IsLocallyControlled())
	{
		UpdateVisualization();

		OnPatrolRoutesChanged.Broadcast();
	}
}

void UUnitPatrolComponent::Server_RemovePatrolRouteByID_Implementation(FGuid PatrolID)
{
	int32 IndexToRemove = -1;
	for (int32 i = 0; i < ActivePatrolRoutes.Num(); ++i)
	{
		if (ActivePatrolRoutes[i].PatrolID == PatrolID)
		{
			IndexToRemove = i;
			break;
		}
	}

	if (IndexToRemove != -1)
	{
		Server_RemovePatrolRoute(IndexToRemove);
		UE_LOG(LogTemp, Log, TEXT("[UnitPatrolComponent] Removed Patrol ID: %s"), *PatrolID.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[UnitPatrolComponent] Could not find Patrol ID: %s to remove"), *PatrolID.ToString());
	}
}

void UUnitPatrolComponent::Server_RemovePatrolRouteForUnit_Implementation(AActor* Unit)
{
	if (!Unit) return;

	// Remove from Cache
	UnitRouteCache.Remove(Unit);

	// Remove from ActivePatrolRoutes
	for (int32 i = ActivePatrolRoutes.Num() - 1; i >= 0; --i)
	{
		FPatrolRoute& Route = ActivePatrolRoutes[i];
		int32 RemovedCount = Route.AssignedUnits.Remove(Unit);
		
		if (RemovedCount > 0)
		{
			// If patrol is now empty, remove it completely
			if (Route.AssignedUnits.Num() == 0)
			{
				Server_RemovePatrolRoute(i);
			}
			else
			{
				// Otherwise just update clients about this change
				if (IsLocallyControlled())
				{
					UpdateVisualization();
					OnPatrolRoutesChanged.Broadcast();
				}
			}
		}
	}
	
	RefreshRoutesFromSelection();
}

void UUnitPatrolComponent::Server_ReversePatrolRoute_Implementation(int32 Index)
{
	if (ActivePatrolRoutes.IsValidIndex(Index))
	{
		Algo::Reverse(ActivePatrolRoutes[Index].PatrolPoints);
		
		const FPatrolRoute& ReversedRoute = ActivePatrolRoutes[Index];
		for (TObjectPtr UnitPtr : ReversedRoute.AssignedUnits)
		{
			if (AActor* Unit = UnitPtr.Get())
			{
				UnitRouteCache.Add(Unit, ReversedRoute);
			}
		}

		if (IsLocallyControlled())
		{
			UpdateVisualization();
		}
	}
}

void UUnitPatrolComponent::Server_ReversePatrolRouteForUnit_Implementation(AActor* Unit)
{
	if (!Unit)
		return;

	if (FPatrolRoute* Route = UnitRouteCache.Find(Unit))
	{
		Algo::Reverse(Route->PatrolPoints);
		RefreshRoutesFromSelection();
	}
}

bool UUnitPatrolComponent::GetPatrolRouteForUnit(AActor* Unit, FPatrolRoute& OutRoute) const
{
	if (!Unit || !Unit->Implements<USelectable>())
		return false;

	if (const FPatrolRoute* Cached = UnitRouteCache.Find(Unit))
	{
		OutRoute = *Cached;
		UE_LOG(LogTemp, Verbose, TEXT("[UnitPatrolComponent] Using cached route for %s with ID %s"), *Unit->GetName(), *Cached->PatrolID.ToString());

		return true;
	}
	
	return false;
}

// ============================================================
// NEW API & HELPERS
// ============================================================

void UUnitPatrolComponent::StopUnits(const TArray<AActor*>& Units)
{
	for (AActor* Unit : Units)
	{
		StopUnit(Unit);
	}
}

void UUnitPatrolComponent::StopUnit(AActor* Unit)
{
	if (!Unit)
		return;

	if (UCommandComponent* CommandComp = Unit->FindComponentByClass<UCommandComponent>())
	{
		FCommandData StopData;
		StopData.Type = CommandMove;
		StopData.Location = Unit->GetActorLocation();
		StopData.SourceLocation = Unit->GetActorLocation();
		StopData.PatrolID.Invalidate(); 
		
		CommandComp->CommandMoveToLocation(StopData);
	}
	else if (UUnitOrderComponent* UnitOrderComp = Unit->FindComponentByClass<UUnitOrderComponent>())
	{
		FCommandData StopData;
		StopData.Type = CommandMove;
		StopData.Location = Unit->GetActorLocation();
		StopData.SourceLocation = Unit->GetActorLocation();
		StopData.PatrolID.Invalidate(); 
		UnitOrderComp->IssueOrder(StopData);
	}
}

void UUnitPatrolComponent::Server_CreatePatrol_Implementation(const FPatrolCreationParams& Params)
{
	FPatrolRoute NewRoute;
	NewRoute.PatrolID = FGuid::NewGuid();
	NewRoute.PatrolPoints = Params.Points;
	NewRoute.PatrolType = Params.Type;
	NewRoute.RouteName = Params.Name;
	NewRoute.RouteColor = Params.Color;
	NewRoute.bShowArrows = Params.bShowArrows;
	NewRoute.bShowNumbers = Params.bShowNumbers;
	
	for (AActor* Unit : Params.Units)
	{
		if (Unit)
			NewRoute.AssignedUnits.AddUnique(Unit);
	}

	if (NewRoute.PatrolPoints.Num() < 2)
		return;

	ActivePatrolRoutes.Add(NewRoute);

	for (TObjectPtr UnitPtr : NewRoute.AssignedUnits)
	{
		if (AActor* Unit = UnitPtr.Get())
		{
			UnitRouteCache.Add(Unit, NewRoute);

			if (UUnitOrderComponent* UnitOrderComp = Unit->FindComponentByClass<UUnitOrderComponent>())
			{
				FCommandData Data;
				Data.Type = CommandPatrol;
				Data.PatrolID = NewRoute.PatrolID;
				Data.PatrolPath = NewRoute.PatrolPoints;
				Data.bPatrolLoop = (NewRoute.PatrolType == EPatrolType::Loop); 
				UnitOrderComp->IssueOrder(Data);
			}
		}
	}

	if (IsLocallyControlled())
	{
		UpdateVisualization();
		OnPatrolRoutesChanged.Broadcast();
	}
}

void UUnitPatrolComponent::EnsureVisualizerComponent()
{
	if (PatrolVisualizer && PatrolVisualizer->IsValidLowLevel())
		return;

	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	PatrolVisualizer = Owner->FindComponentByClass<UPatrolVisualizerComponent>();
	if (!PatrolVisualizer)
	{
		PatrolVisualizer = NewObject<UPatrolVisualizerComponent>(Owner, UPatrolVisualizerComponent::StaticClass(), TEXT("PatrolVisualizer"));
		if (PatrolVisualizer)
		{
			PatrolVisualizer->RegisterComponent();
			UpdateVisualization();
		}
	}
}

void UUnitPatrolComponent::UpdateVisualization()
{
	if (!IsLocallyControlled())
		return;

	EnsureVisualizerComponent();
	
	if (!PatrolVisualizer)
		return;

	TArray<FPatrolRouteExtended> ExtendedRoutes;

	for (const FPatrolRoute& Route : ActivePatrolRoutes)
	{
		if (Route.PatrolPoints.Num() >= 2)
		{
			ExtendedRoutes.Add(ConvertToExtended(Route, RouteColor));
		}
	}

	if (ExtendedRoutes.Num() > 0)
	{
		PatrolVisualizer->SetVisibility(true);
	}
	
	PatrolVisualizer->UpdateVisualization(ExtendedRoutes);
}

FPatrolRouteExtended UUnitPatrolComponent::ConvertToExtended(const FPatrolRoute& Route, const FLinearColor& Color)
{
	FPatrolRouteExtended Extended;
	Extended.PatrolPoints = Route.PatrolPoints;
	Extended.PatrolType = Route.PatrolType;
	Extended.RouteColor = Route.RouteColor;
	Extended.RouteName = Route.RouteName;
	Extended.WaitTimeAtWaypoints = Route.WaitTime;
	Extended.bShowDirectionArrows = Route.bShowArrows;
	Extended.bShowWaypointNumbers = Route.bShowNumbers;
	
	return Extended;
}

void UUnitPatrolComponent::Server_RenamePatrol_Implementation(FGuid PatrolID, FName NewName)
{
	for (int32 i = 0; i < ActivePatrolRoutes.Num(); ++i)
	{
		if (ActivePatrolRoutes[i].PatrolID == PatrolID)
		{
			ActivePatrolRoutes[i].RouteName = NewName;
			OnRep_ActivePatrolRoutes();
			break;
		}
	}
}

int32 UUnitPatrolComponent::GetTargetPointIndexForUnit(AActor* Unit, int32 NewPathSize, bool bIsReverse) const
{
	if (NewPathSize == 0 || !Unit)
		return 0;

	if (APawn* Pawn = Cast<APawn>(Unit))
	{
		if (AAiControllerRts* AI = Cast<AAiControllerRts>(Pawn->GetController()))
		{
			int32 CurrentIndex = AI->GetCurrentPatrolWaypointIndex();
			
			// If Reversing:
			// The unit was heading to 'CurrentIndex' (e.g., Index 1 in A->B->C).
			// We want to return to where we came from (Index 0, A).
			// In the reversed path (C->B->A), A is at index (N-1) - PreviousIndex.
			// PreviousIndex = CurrentIndex - 1 (roughly).
			
			if (bIsReverse)
			{
				if (CurrentIndex > 0)
				{
					return (NewPathSize - CurrentIndex);
				}
				else
				{
					// Unit was going to Start (0). Previous point was End (Loop) or None.
					// Mapping 0 -> 0 is the safest fallback for Reverse-at-Start.
					return 0;
				}
			}
			else
			{
				// Standard Update: Keep logical target index.
				return FMath::Clamp(CurrentIndex, 0, NewPathSize - 1);
			}
		}
	}

	return 0;
}

void UUnitPatrolComponent::UpdatePatrolOrderForUnit(AActor* Unit, const FPatrolRoute& Route, bool bIsReverseUpdate)
{
	if (!Unit) return;

	// Update Cache
	UnitRouteCache.Add(Unit, Route);

	if (UUnitOrderComponent* UnitOrderComp = Unit->FindComponentByClass<UUnitOrderComponent>())
	{
		FCommandData Data;
		Data.Type = CommandPatrol;
		Data.PatrolID = Route.PatrolID;
		Data.PatrolPath = Route.PatrolPoints;
		
		// PingPong is NOT a circular loop for the movement system.
		Data.bPatrolLoop = (Route.PatrolType == EPatrolType::Loop); 
		
		Data.StartIndex = GetTargetPointIndexForUnit(Unit, Data.PatrolPath.Num(), bIsReverseUpdate);

		UnitOrderComp->IssueOrder(Data);
		UE_LOG(LogTemp, Verbose, TEXT("[UnitPatrolComponent] Updated patrol for %s. StartIndex: %d, Loop: %d"), 
			*Unit->GetName(), Data.StartIndex, Data.bPatrolLoop);
	}
}

void UUnitPatrolComponent::Server_ReversePatrolRouteByID_Implementation(FGuid PatrolID)
{
    for (int32 i = 0; i < ActivePatrolRoutes.Num(); ++i)
	{
		if (ActivePatrolRoutes[i].PatrolID == PatrolID)
		{
            Algo::Reverse(ActivePatrolRoutes[i].PatrolPoints);
            
			OnRep_ActivePatrolRoutes();
            
            // Re-issue orders
            for (TObjectPtr<AActor> UnitPtr : ActivePatrolRoutes[i].AssignedUnits)
            {
                if (AActor* Unit = UnitPtr.Get())
                {
                    UpdatePatrolOrderForUnit(Unit, ActivePatrolRoutes[i], true);
                }
            }
			break;
		}
	}
}

void UUnitPatrolComponent::Server_SetPatrolType_Implementation(FGuid PatrolID, EPatrolType NewType)
{
    for (int32 i = 0; i < ActivePatrolRoutes.Num(); ++i)
	{
		if (ActivePatrolRoutes[i].PatrolID == PatrolID)
		{
            ActivePatrolRoutes[i].PatrolType = NewType;

			OnRep_ActivePatrolRoutes();
            
             // Re-issue orders
            for (TObjectPtr<AActor> UnitPtr : ActivePatrolRoutes[i].AssignedUnits)
            {
                if (AActor* Unit = UnitPtr.Get())
                {
                    UpdatePatrolOrderForUnit(Unit, ActivePatrolRoutes[i], false);
                }
            }
			break;
		}
	}
}
