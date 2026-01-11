#include "Components/Patrol/UnitPatrolComponent.h"
#include "Components/Patrol/PatrolVisualizerComponent.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "Algo/Reverse.h"
#include "Interfaces/Selectable.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "AI/AiControllerRts.h"


UUnitPatrolComponent::UUnitPatrolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
	SetIsReplicatedByDefault(true);
}

void FPatrolRouteItem::PreReplicatedRemove(const FPatrolRouteArray& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->OnPatrolRouteRemoved(RouteData);
	}
}

void FPatrolRouteItem::PostReplicatedAdd(const FPatrolRouteArray& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->OnPatrolRouteAdded(RouteData);
	}
}

void FPatrolRouteItem::PostReplicatedChange(const FPatrolRouteArray& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->OnPatrolRouteChanged(RouteData);
	}
}

void UUnitPatrolComponent::BeginPlay()
{
	Super::BeginPlay();

	PatrolRoutes.OwnerComponent = this;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
		return;

	SelectionComponent = OwnerActor->FindComponentByClass<UUnitSelectionComponent>();

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
	DOREPLIFETIME(UUnitPatrolComponent, PatrolRoutes);
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

// (Orphaned lines removed)

// ... 

void UUnitPatrolComponent::OnSelectionChanged(const TArray<AActor*>& SelectedActors)
{
	RefreshRoutesFromSelection();
}



void UUnitPatrolComponent::ApplyRoutes(const TArray<FPatrolRoute>& NewRoutes)
{
	if (IsLocallyControlled())
	{
		UpdateVisualization();
		OnPatrolRoutesChanged.Broadcast();
	}
}

// ============================================================
// CALLBACK IMPLEMENTATION
// ============================================================

void UUnitPatrolComponent::OnPatrolRouteAdded(const FPatrolRoute& Route)
{
    RebuildUnitLookupMap();

    if (IsLocallyControlled())
    {
        UpdateVisualization();
        OnPatrolRoutesChanged.Broadcast();
    }
}

void UUnitPatrolComponent::OnPatrolRouteChanged(const FPatrolRoute& Route)
{
    RebuildUnitLookupMap();

    if (IsLocallyControlled())
    {
        UpdateVisualization();
        OnPatrolRoutesChanged.Broadcast();
    }
}

void UUnitPatrolComponent::OnPatrolRouteRemoved(const FPatrolRoute& Route)
{
    RebuildUnitLookupMap();

    if (HasAuthority())
    {
        for (TObjectPtr UnitPtr : Route.AssignedUnits)
        {
            if (AActor* Unit = UnitPtr.Get())
            {
                if (APawn* Pawn = Cast<APawn>(Unit))
                {
                    if (AAiControllerRts* AI = Cast<AAiControllerRts>(Pawn->GetController()))
                    {
                        AI->StopPatrol();
                    }
                }
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
	if (PatrolRoutes.Items.IsValidIndex(Index))
	{
		PatrolRoutes.Items[Index].RouteData = NewRoute;
		PatrolRoutes.MarkItemDirty(PatrolRoutes.Items[Index]);
		
		UE_LOG(LogTemp, Log, TEXT("[UnitPatrolComponent] Updating patrol route %d (ID: %s) for %d units"), 
			Index, *NewRoute.PatrolID.ToString(), NewRoute.AssignedUnits.Num());

        // Notify AI (Standard Update)
        NotifyPatrolUpdate(NewRoute, false);

		OnPatrolRouteChanged(NewRoute);
	}
}



void UUnitPatrolComponent::Server_RemovePatrolRoute_Implementation(int32 Index)
{
	if (PatrolRoutes.Items.IsValidIndex(Index))
	{
		const FPatrolRoute RouteToRemove = PatrolRoutes.Items[Index].RouteData;
		FGuid RemovedID = RouteToRemove.PatrolID;

		PatrolRoutes.Items.RemoveAt(Index);
		PatrolRoutes.MarkArrayDirty();
		
		OnPatrolRouteRemoved(RouteToRemove);
		
		Multicast_RemovePatrolRoute(TArray<AActor*>(), RemovedID);
	}
}

void UUnitPatrolComponent::Multicast_RemovePatrolRoute_Implementation(const TArray<AActor*>& Units, const FGuid& PatrolID)
{
	for (int32 i = PatrolRoutes.Items.Num() - 1; i >= 0; --i)
	{
		if (PatrolRoutes.Items[i].RouteData.PatrolID == PatrolID)
		{
			PatrolRoutes.Items.RemoveAt(i);
			PatrolRoutes.MarkArrayDirty();
		}
	}
    
    RebuildUnitLookupMap();

	if (IsLocallyControlled())
	{
		UpdateVisualization();
		OnPatrolRoutesChanged.Broadcast();
	}
}

void UUnitPatrolComponent::Server_RemovePatrolRouteByID_Implementation(FGuid PatrolID)
{
	int32 IndexToRemove = -1;
	for (int32 i = 0; i < PatrolRoutes.Items.Num(); ++i)
	{
		if (PatrolRoutes.Items[i].RouteData.PatrolID == PatrolID)
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
	if (!Unit)
		return;

	if (!Unit)
		return;

	for (int32 i = PatrolRoutes.Items.Num() - 1; i >= 0; --i)
	{
		FPatrolRoute& Route = PatrolRoutes.Items[i].RouteData;
		int32 RemovedCount = Route.AssignedUnits.Remove(Unit);
		
		if (RemovedCount > 0)
		{
			PatrolRoutes.MarkItemDirty(PatrolRoutes.Items[i]);
			
			if (Route.AssignedUnits.Num() == 0)
			{
				Server_RemovePatrolRoute(i);
			}
			else
			{
				OnPatrolRouteChanged(Route);
			}
		}
	}
	
	RefreshRoutesFromSelection();
}

void UUnitPatrolComponent::Server_ReversePatrolRoute_Implementation(int32 Index)
{
	if (PatrolRoutes.Items.IsValidIndex(Index))
	{
		Algo::Reverse(PatrolRoutes.Items[Index].RouteData.PatrolPoints);
		
        const FPatrolRoute& RevisedRoute = PatrolRoutes.Items[Index].RouteData;
		PatrolRoutes.MarkItemDirty(PatrolRoutes.Items[Index]);
        
        NotifyPatrolUpdate(RevisedRoute, true);
		OnPatrolRouteChanged(RevisedRoute);
	}
}

void UUnitPatrolComponent::Server_ReversePatrolRouteForUnit_Implementation(AActor* Unit)
{
	if (!Unit)
		return;

	if (FGuid* RouteID = UnitToRouteMap.Find(Unit))
	{
        for (int32 i = 0; i < PatrolRoutes.Items.Num(); ++i)
        {
            if (PatrolRoutes.Items[i].RouteData.PatrolID == *RouteID)
            {
		        Algo::Reverse(PatrolRoutes.Items[i].RouteData.PatrolPoints);
				
                const FPatrolRoute& Route = PatrolRoutes.Items[i].RouteData;
                PatrolRoutes.MarkItemDirty(PatrolRoutes.Items[i]);

                NotifyPatrolUpdate(Route, true);
                OnPatrolRouteChanged(Route);
                break;
            }
        }
	}
}

bool UUnitPatrolComponent::GetPatrolRouteForUnit(AActor* Unit, FPatrolRoute& OutRoute) const
{
	if (!Unit || !Unit->Implements<USelectable>())
		return false;

	if (const FGuid* ID = UnitToRouteMap.Find(Unit))
	{
         for (const FPatrolRouteItem& Item : PatrolRoutes.Items)
         {
             if (Item.RouteData.PatrolID == *ID)
             {
		         OutRoute = Item.RouteData;
		         UE_LOG(LogTemp, Verbose, TEXT("[UnitPatrolComponent] Using cached route ID lookup for %s"), *Unit->GetName());
		         return true;
             }
         }
	}
	
	return false;
}

void UUnitPatrolComponent::RebuildUnitLookupMap()
{
    UnitToRouteMap.Empty();
    for (const FPatrolRouteItem& Item : PatrolRoutes.Items)
    {
        for (const TObjectPtr<AActor>& UnitPtr : Item.RouteData.AssignedUnits)
        {
            if (AActor* Unit = UnitPtr.Get())
            {
                if (IsValid(Unit))
                {
                    UnitToRouteMap.Add(Unit, Item.RouteData.PatrolID);
                }
            }
        }
    }
}

// ============================================================
// NEW API & HELPERS
// ============================================================

void UUnitPatrolComponent::Server_CreatePatrol_Implementation(const FPatrolCreationParams& Params)
{
	CreatePatrol(Params);
}

FGuid UUnitPatrolComponent::CreatePatrol(const FPatrolCreationParams& Params)
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
		return FGuid();

	FPatrolRouteItem& NewItem = PatrolRoutes.Items.Add_GetRef(FPatrolRouteItem(NewRoute));
	PatrolRoutes.MarkItemDirty(NewItem);
	PatrolRoutes.MarkArrayDirty();
	
	OnPatrolRouteAdded(NewRoute);

	return NewRoute.PatrolID;
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

	for (const FPatrolRouteItem& Item : PatrolRoutes.Items)
	{
		const FPatrolRoute& Route = Item.RouteData;
		if (Route.PatrolPoints.Num() >= 2)
		{
            bool bIsRouteVisible = false;

            if (SelectionComponent)
            {
                for (AActor* Unit : Route.AssignedUnits)
                {
                    if (SelectionComponent->ActorSelected(Unit))
                    {
                        bIsRouteVisible = true;
                        break;
                    }
                }
            }

            if (bIsRouteVisible)
            {
			    ExtendedRoutes.Add(ConvertToExtended(Route, RouteColor));
            }
		}
	}

	if (ExtendedRoutes.Num() > 0)
	{
		PatrolVisualizer->SetVisibility(true);
	}
    else
    {
        PatrolVisualizer->SetVisibility(false);
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

void UUnitPatrolComponent::Server_ModifyPatrol_Implementation(FGuid PatrolID, EPatrolModAction Action, const FPatrolModPayload& Payload)
{
    for (int32 i = 0; i < PatrolRoutes.Items.Num(); ++i)
    {
        if (PatrolRoutes.Items[i].RouteData.PatrolID == PatrolID)
        {
            FPatrolRoute& Route = PatrolRoutes.Items[i].RouteData;
            bool bIsReverse = false;

            // Apply Modification
            switch (Action)
            {
                case EPatrolModAction::Rename:
                    Route.RouteName = Payload.NewName;
                    break;
                case EPatrolModAction::ChangeType:
                    Route.PatrolType = Payload.NewType;
                    break;
                case EPatrolModAction::ChangeColor:
                    Route.RouteColor = Payload.NewColor;
                    break;
                case EPatrolModAction::Reverse:
                    Algo::Reverse(Route.PatrolPoints);
                    bIsReverse = true;
                    break;
            }

            PatrolRoutes.MarkItemDirty(PatrolRoutes.Items[i]);
            NotifyPatrolUpdate(Route, bIsReverse);
            OnPatrolRouteChanged(Route);

            UE_LOG(LogTemp, Log, TEXT("[UnitPatrolComponent] Modified Patrol %s: Action %d"), *PatrolID.ToString(), (int32)Action);
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
			
			if (bIsReverse)
			{
				if (CurrentIndex > 0)
				{
					return (NewPathSize - CurrentIndex);
				}
				
				return 0;
			}
			
			return FMath::Clamp(CurrentIndex, 0, NewPathSize - 1);
		}
	}

	return 0;
}


void UUnitPatrolComponent::NotifyPatrolUpdate(const FPatrolRoute& Route, bool bIsReverse)
{
    if (!HasAuthority())
        return;

    const bool bLoop = (Route.PatrolType == EPatrolType::Loop);
    const int32 NumPoints = Route.PatrolPoints.Num();

    for (TObjectPtr UnitPtr : Route.AssignedUnits)
    {
        AActor* Unit = UnitPtr.Get();
        if (APawn* Pawn = Cast<APawn>(Unit))
        {
            if (AAiControllerRts* AI = Cast<AAiControllerRts>(Pawn->GetController()))
            {
                int32 NewStartIndex = GetTargetPointIndexForUnit(Unit, NumPoints, bIsReverse);
                AI->UpdateCurrentPatrol(Route.PatrolPoints, bLoop, NewStartIndex);
            }
        }
    }
}
