#include "Components/UnitPatrolComponent.h"
#include "Components/PatrolVisualizerComponent.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Interfaces/Selectable.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"


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

void UUnitPatrolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UUnitPatrolComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
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
			// Check if we already have this patrol ID in NewRoutes
			bool bFound = false;
			for (FPatrolRoute& ExistingRoute : NewRoutes)
			{
				// Group by PatrolID if valid
				if (ExistingRoute.PatrolID.IsValid() && ExistingRoute.PatrolID == Route.PatrolID)
				{
					ExistingRoute.AssignedUnits.AddUnique(Unit);
					bFound = true;
					UE_LOG(LogTemp, Verbose, TEXT("[UnitPatrolComponent] Unit %s added to existing patrol %s"), 
						*Unit->GetName(), *Route.PatrolID.ToString());
					break;
				}
			}

			if (!bFound)
			{
				Route.AssignedUnits.Add(Unit);
				NewRoutes.Add(Route);
				UE_LOG(LogTemp, Verbose, TEXT("[UnitPatrolComponent] New patrol route added with ID %s"), 
					*Route.PatrolID.ToString());
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("RefreshRoutesFromSelection: Found %d unique routes from %d units"), NewRoutes.Num(), SelectedUnits.Num());
	ApplyRoutes(NewRoutes);
}

void UUnitPatrolComponent::HandleOrdersDispatched(const TArray<AActor*>& AffectedUnits, const FCommandData& IssuedCommand)
{
	if (IssuedCommand.Type != ECommandType::CommandPatrol || IssuedCommand.PatrolPath.Num() < 2)
		return;

	// Create a single shared route for all affected units
	FPatrolRoute SharedRoute;
	SharedRoute.PatrolID = FGuid::NewGuid(); // Generate unique ID for this patrol
	SharedRoute.PatrolPoints = IssuedCommand.PatrolPath;
	SharedRoute.PatrolType = IssuedCommand.bPatrolLoop ? EPatrolType::Loop : EPatrolType::Once;

	// Check if we already have cached data for any of these units
	// If so, reuse the metadata (name, color, etc.) BUT keep the new ID
	bool bFoundCache = false;
	for (AActor* Unit : AffectedUnits)
	{
		if (!Unit)
			continue;

		if (const FPatrolRoute* Cached = UnitRouteCache.Find(Unit))
		{
			SharedRoute.RouteName = Cached->RouteName;
			SharedRoute.RouteColor = Cached->RouteColor;
			SharedRoute.WaitTime = Cached->WaitTime;
			SharedRoute.bShowArrows = Cached->bShowArrows;
			SharedRoute.bShowNumbers = Cached->bShowNumbers;
			SharedRoute.PatrolType = Cached->PatrolType;
			bFoundCache = true;
			break; // Use the first found cache
		}
	}

	// If no cache found, use default color
	if (!bFoundCache)
	{
		SharedRoute.RouteColor = RouteColor;
	}

	UE_LOG(LogTemp, Log, TEXT("[UnitPatrolComponent] Created new patrol with ID: %s for %d units"), 
		*SharedRoute.PatrolID.ToString(), AffectedUnits.Num());

	// Apply the same shared route to all units in the cache
	for (AActor* Unit : AffectedUnits)
	{
		if (!Unit)
			continue;
		UnitRouteCache.Add(Unit, SharedRoute);
	}

	RefreshRoutesFromSelection();
}

void UUnitPatrolComponent::OnSelectionChanged(const TArray<AActor*>& SelectedActors)
{
	RefreshRoutesFromSelection();
}



void UUnitPatrolComponent::ApplyRoutes(const TArray<FPatrolRoute>& NewRoutes)
{
	// Always update if we have new metadata (Color/Name) even if geometry is same
	// The AreRoutesEquivalent check was too strict on geometry only, or ignored metadata.
	// Let's just update.
	ActivePatrolRoutes = NewRoutes;
	
	if (IsLocallyControlled())
	{
		UpdateVisualization();
	}
}

void UUnitPatrolComponent::OnRep_ActivePatrolRoutes()
{
	if (IsLocallyControlled())
	{
		UpdateVisualization();
	}
}

void UUnitPatrolComponent::Server_UpdatePatrolRoute_Implementation(int32 Index, const FPatrolRoute& NewRoute)
{
	if (ActivePatrolRoutes.IsValidIndex(Index))
	{
		// Update the route in the active list
		ActivePatrolRoutes[Index] = NewRoute;
		
		UE_LOG(LogTemp, Log, TEXT("[UnitPatrolComponent] Updating patrol route %d (ID: %s) for %d units"), 
			Index, *NewRoute.PatrolID.ToString(), NewRoute.AssignedUnits.Num());
		
		// Propagate changes to all assigned units in the cache AND re-issue orders
		for (TObjectPtr<AActor> UnitPtr : NewRoute.AssignedUnits)
		{
			if (AActor* Unit = UnitPtr.Get())
			{
				UnitRouteCache.Add(Unit, NewRoute);
				
				// Re-issue the order to update behavior immediately
				if (UUnitOrderComponent* UnitOrderComp = Unit->FindComponentByClass<UUnitOrderComponent>())
				{
					FCommandData Data;
					Data.Type = ECommandType::CommandPatrol;
					Data.PatrolID = NewRoute.PatrolID; // Include patrol ID
					Data.PatrolPath = NewRoute.PatrolPoints;
					Data.bPatrolLoop = (NewRoute.PatrolType == EPatrolType::Loop || NewRoute.PatrolType == EPatrolType::PingPong); 
					
					UnitOrderComp->IssueOrder(Data);
					UE_LOG(LogTemp, Log, TEXT("[UnitPatrolComponent] Re-issued patrol order to %s"), *Unit->GetName());
				}
			}
		}

		// Force update on server (if listen server) and trigger replication
		if (IsLocallyControlled())
		{
			UpdateVisualization();
		}
	}
}

void UUnitPatrolComponent::Server_UpdateUnitPatrol_Implementation(AActor* Unit, const FPatrolRoute& NewRoute)
{
	if (!Unit) return;

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
		
		// Remove from cache for all assigned units
		for (TObjectPtr<AActor> UnitPtr : RouteToRemove.AssignedUnits)
		{
			if (AActor* Unit = UnitPtr.Get())
			{
				UnitRouteCache.Remove(Unit);
				// Optionally: Stop the unit or clear its command?
				// For now, we just remove the patrol metadata.
			}
		}

		ActivePatrolRoutes.RemoveAt(Index);
		
		if (IsLocallyControlled())
		{
			UpdateVisualization();
		}
	}
}

void UUnitPatrolComponent::Server_RemovePatrolRouteForUnit_Implementation(AActor* Unit)
{
	if (!Unit) return;

	if (UnitRouteCache.Remove(Unit) > 0)
	{
		RefreshRoutesFromSelection();
	}
}

void UUnitPatrolComponent::Server_ReversePatrolRoute_Implementation(int32 Index)
{
	if (ActivePatrolRoutes.IsValidIndex(Index))
	{
		Algo::Reverse(ActivePatrolRoutes[Index].PatrolPoints);
		
		// Update cache for all assigned units with the reversed points
		const FPatrolRoute& ReversedRoute = ActivePatrolRoutes[Index];
		for (TObjectPtr<AActor> UnitPtr : ReversedRoute.AssignedUnits)
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
	if (!Unit) return;

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

	// PRIORITY 1: Check cache FIRST (contains user modifications)
	if (const FPatrolRoute* Cached = UnitRouteCache.Find(Unit))
	{
		OutRoute = *Cached;
		UE_LOG(LogTemp, Verbose, TEXT("[UnitPatrolComponent] Using cached route for %s with ID %s"), 
			*Unit->GetName(), *Cached->PatrolID.ToString());
		return true;
	}

	// PRIORITY 2: Fall back to active command if no cache
	const FCommandData CurrentCommand = ISelectable::Execute_GetCurrentCommand(Unit);
	if (CurrentCommand.Type != ECommandType::CommandPatrol || CurrentCommand.PatrolPath.Num() < 2)
		return false;

	// Build route from command data
	OutRoute.PatrolID = CurrentCommand.PatrolID;
	OutRoute.PatrolPoints = CurrentCommand.PatrolPath;
	OutRoute.PatrolType = CurrentCommand.bPatrolLoop ? EPatrolType::Loop : EPatrolType::Once;
	OutRoute.RouteColor = RouteColor;
	
	UE_LOG(LogTemp, Verbose, TEXT("[UnitPatrolComponent] Using command route for %s with ID %s"), 
		*Unit->GetName(), *CurrentCommand.PatrolID.ToString());
	
	return true;
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
	Extended.RouteColor = Route.RouteColor; // Use the route's specific color
	Extended.RouteName = Route.RouteName;
	Extended.WaitTimeAtWaypoints = Route.WaitTime;
	Extended.bShowDirectionArrows = Route.bShowArrows;
	Extended.bShowWaypointNumbers = Route.bShowNumbers;
	return Extended;
}
