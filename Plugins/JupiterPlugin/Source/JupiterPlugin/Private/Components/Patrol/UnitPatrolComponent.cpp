#include "Components/Patrol/UnitPatrolComponent.h"
#include "Components/Patrol/PatrolVisualizerComponent.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "Algo/Reverse.h"
#include "Interfaces/Selectable.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/PatrolAgentInterface.h"


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

void UUnitPatrolComponent::OnUnitDestroyed(AActor* DestroyedActor)
{
    UnitToRouteMap.Remove(DestroyedActor);
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

void UUnitPatrolComponent::OnRep_ActivePatrolRoutes()
{
	RebuildUnitLookupMap();
	
	if (IsLocallyControlled())
	{
		UpdateVisualization();
		OnPatrolRoutesChanged.Broadcast();
	}
}

// ============================================================
// CALLBACK
// ============================================================

void UUnitPatrolComponent::OnPatrolRouteAdded(const FPatrolRoute& Route)
{
    for (const TObjectPtr<AActor>& UnitPtr : Route.AssignedUnits)
    {
        if (AActor* Unit = UnitPtr.Get())
        {
            if (IsValid(Unit))
            {
                 UnitToRouteMap.Add(Unit, Route.PatrolID);
                 if (!Unit->OnDestroyed.IsAlreadyBound(this, &UUnitPatrolComponent::OnUnitDestroyed))
                 {
                     Unit->OnDestroyed.AddDynamic(this, &UUnitPatrolComponent::OnUnitDestroyed);
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

void UUnitPatrolComponent::OnPatrolRouteChanged(const FPatrolRoute& Route)
{
    TSet<AActor*> CurrentUnits;
    for (const auto& UnitPtr : Route.AssignedUnits)
    {
        if (AActor* Unit = UnitPtr.Get())
        {
             CurrentUnits.Add(Unit);
             UnitToRouteMap.Add(Unit, Route.PatrolID);
             if (!Unit->OnDestroyed.IsAlreadyBound(this, &UUnitPatrolComponent::OnUnitDestroyed))
             {
                 Unit->OnDestroyed.AddDynamic(this, &UUnitPatrolComponent::OnUnitDestroyed);
             }
        }
    }

    for (auto It = UnitToRouteMap.CreateIterator(); It; ++It)
    {
        if (It.Value() == Route.PatrolID)
        {
            AActor* Unit = It.Key().Get();
            if (!Unit || !CurrentUnits.Contains(Unit))
            {
                It.RemoveCurrent();
            }
        }
    }

    if (IsLocallyControlled())
    {
        UpdateVisualization();
        OnPatrolRoutesChanged.Broadcast();
    }
}

void UUnitPatrolComponent::OnPatrolRouteRemoved(const FPatrolRoute& Route)
{
    for (const TObjectPtr<AActor>& UnitPtr : Route.AssignedUnits)
    {
        if (AActor* Unit = UnitPtr.Get())
        {
             if (FGuid* FoundID = UnitToRouteMap.Find(Unit))
             {
                 if (*FoundID == Route.PatrolID)
                 {
                     UnitToRouteMap.Remove(Unit);
                     Unit->OnDestroyed.RemoveDynamic(this, &UUnitPatrolComponent::OnUnitDestroyed);
                 }
             }
        }
    }

    if (HasAuthority())
    {
        for (TObjectPtr UnitPtr : Route.AssignedUnits)
        {
            if (AActor* Unit = UnitPtr.Get())
            {
	            if (Unit->Implements<UPatrolAgentInterface>())
	            {
		             if (IPatrolAgentInterface* Agent = Cast<IPatrolAgentInterface>(Unit))
		             {
			             Agent->StopPatrol();
		             }
	            }
                else if (APawn* Pawn = Cast<APawn>(Unit))
                {
                    if (IPatrolAgentInterface* AgentController = Cast<IPatrolAgentInterface>(Pawn->GetController()))
                    {
                        AgentController->StopPatrol();
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

void UUnitPatrolComponent::Server_AssignUnitsToPatrol_Implementation(const TArray<AActor*>& Units, FGuid PatrolID)
{
    int32 RouteIndex = -1;
    for (int32 i = 0; i < PatrolRoutes.Items.Num(); ++i)
    {
        if (PatrolRoutes.Items[i].RouteData.PatrolID == PatrolID)
        {
            RouteIndex = i;
            break;
        }
    }

    if (RouteIndex == INDEX_NONE)
    {
        return;
    }

    FPatrolRoute& RouteData = PatrolRoutes.Items[RouteIndex].RouteData;
    if (RouteData.PatrolPoints.Num() < 2) 
        return;
        
    APlayerController* PC = Cast<APlayerController>(GetOwner());

    bool bChanged = false;

    for (AActor* Unit : Units)
    {
        if (!IsValid(Unit)) 
			continue;

        if (RouteData.AssignedUnits.Contains(Unit))
        {
            continue;
        }

        Server_RemovePatrolRouteForUnit(Unit);

        RouteData.AssignedUnits.Add(Unit);
        bChanged = true;

        UE_LOG(LogTemp, Warning, TEXT("Server_AssignUnitsToPatrol - Processing Unit: %s"), *Unit->GetName());

             FCommandData Data(PC, RouteData.PatrolPoints[0], FRotator::ZeroRotator, ECommandType::CommandPatrol);
             Data.PatrolPath = RouteData.PatrolPoints;
             Data.bPatrolLoop = (RouteData.PatrolType == EPatrolType::Loop);
             Data.PatrolID = PatrolID;
             
             UE_LOG(LogTemp, Warning, TEXT("Server_AssignUnitsToPatrol - Issuing CommandMove to %s | PatrolID: %s | Points: %d"), 
                *Unit->GetName(), *PatrolID.ToString(), Data.PatrolPath.Num());

             if (Unit->Implements<USelectable>())
             {
                 ISelectable::Execute_CommandMove(Unit, Data);
             }
             else
             {
                 UE_LOG(LogTemp, Error, TEXT("Server_AssignUnitsToPatrol - Unit %s does not implement ISelectable!"), *Unit->GetName());
             }
    }

    if (bChanged)
    {
        PatrolRoutes.MarkItemDirty(PatrolRoutes.Items[RouteIndex]);
        NotifyPatrolUpdate(RouteData, false);
        OnPatrolRouteChanged(RouteData);
    }
}

void UUnitPatrolComponent::Multicast_RemovePatrolRoute_Implementation(const TArray<AActor*>& Units, const FGuid& PatrolID)
{
	for (int32 i = PatrolRoutes.Items.Num() - 1; i >= 0; --i)
	{
		if (PatrolRoutes.Items[i].RouteData.PatrolID == PatrolID)
		{
			const FPatrolRoute RemovedRoute = PatrolRoutes.Items[i].RouteData;
			
			PatrolRoutes.Items.RemoveAt(i);
			PatrolRoutes.MarkArrayDirty();
			
			OnPatrolRouteRemoved(RemovedRoute);
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

void UUnitPatrolComponent::Server_RemovePatrolWithOption_Implementation(FGuid PatrolID, EPatrolDeleteOption Option)
{
	int32 IndexToRemove = -1;
	FPatrolRoute RouteToRemove;
	bool bFound = false;

	for (int32 i = 0; i < PatrolRoutes.Items.Num(); ++i)
	{
		if (PatrolRoutes.Items[i].RouteData.PatrolID == PatrolID)
		{
			IndexToRemove = i;
			RouteToRemove = PatrolRoutes.Items[i].RouteData;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		 UE_LOG(LogTemp, Warning, TEXT("[UnitPatrolComponent] Could not find Patrol ID: %s to remove"), *PatrolID.ToString());
		 return;
	}

	if (Option == EPatrolDeleteOption::JoinNearest && RouteToRemove.AssignedUnits.Num() > 0)
	{
		FVector OldCenter = FVector::ZeroVector;
		int32 ValidPoints = 0;
		if (RouteToRemove.PatrolPoints.Num() > 0)
		{
			for (const FVector& Pt : RouteToRemove.PatrolPoints)
			{
				OldCenter += Pt;
			}
			if (RouteToRemove.PatrolPoints.Num() > 0)
            {
			    OldCenter /= RouteToRemove.PatrolPoints.Num();
            }
		}

		FGuid NearestPatrolID;
		float MinDistSq = MAX_FLT;
		bool bHasNearest = false;

		for (const FPatrolRouteItem& Item : PatrolRoutes.Items)
		{
			if (Item.RouteData.PatrolID == PatrolID) 
				continue;

            if (Item.RouteData.PatrolPoints.Num() == 0) 
				continue;

			FVector NewCenter = FVector::ZeroVector;
			for (const FVector& Pt : Item.RouteData.PatrolPoints)
			{
				NewCenter += Pt;
			}
			NewCenter /= Item.RouteData.PatrolPoints.Num();

			float DistSq = FVector::DistSquared(OldCenter, NewCenter);
			if (DistSq < MinDistSq)
			{
				MinDistSq = DistSq;
				NearestPatrolID = Item.RouteData.PatrolID;
				bHasNearest = true;
			}
		}

		if (bHasNearest)
		{
			TArray<AActor*> UnitsToMove;
			 for (const auto& UnitPtr : RouteToRemove.AssignedUnits)
			{
				if (AActor* Unit = UnitPtr.Get())
				{
					UnitsToMove.Add(Unit);
				}
			}
			
			 for (int32 i = 0; i < PatrolRoutes.Items.Num(); ++i)
			 {
				 if (PatrolRoutes.Items[i].RouteData.PatrolID == NearestPatrolID)
				 {
					  for(AActor* U : UnitsToMove)
					  {
						  PatrolRoutes.Items[i].RouteData.AssignedUnits.AddUnique(U);
					  }
					  
					  PatrolRoutes.MarkItemDirty(PatrolRoutes.Items[i]);
					  NotifyPatrolUpdate(PatrolRoutes.Items[i].RouteData, false);
					  OnPatrolRouteChanged(PatrolRoutes.Items[i].RouteData);
					  
					  UE_LOG(LogTemp, Log, TEXT("[UnitPatrolComponent] Moved %d units to patrol %s"), UnitsToMove.Num(), *NearestPatrolID.ToString());
					  
                      if (PatrolRoutes.Items.IsValidIndex(IndexToRemove))
                      {
                            PatrolRoutes.Items[IndexToRemove].RouteData.AssignedUnits.Empty();
                      }
                      
                      break;
				 }
			 }
		}
	}

	Server_RemovePatrolRoute(IndexToRemove);
}

void UUnitPatrolComponent::Server_RemovePatrolRouteForUnit_Implementation(AActor* Unit)
{
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

void UUnitPatrolComponent::Server_UpdatePatrolPoint_Implementation(FGuid PatrolID, int32 PointIndex, FVector NewLocation)
{
    for (int32 i = 0; i < PatrolRoutes.Items.Num(); ++i)
    {
        if (PatrolRoutes.Items[i].RouteData.PatrolID == PatrolID)
        {
            if (PatrolRoutes.Items[i].RouteData.PatrolPoints.IsValidIndex(PointIndex))
            {
                PatrolRoutes.Items[i].RouteData.PatrolPoints[PointIndex] = NewLocation;

                PatrolRoutes.MarkItemDirty(PatrolRoutes.Items[i]);
                const FPatrolRoute& Route = PatrolRoutes.Items[i].RouteData;

                OnPatrolRouteChanged(Route);
                NotifyPatrolUpdate(Route, false);
            }
			
            break;
        }
    }
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
                    if (!Unit->OnDestroyed.IsAlreadyBound(this, &UUnitPatrolComponent::OnUnitDestroyed))
                    {
                        Unit->OnDestroyed.AddDynamic(this, &UUnitPatrolComponent::OnUnitDestroyed);
                    }
                }
            }
        }
    }
}

// ============================================================
// HELPERS
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
        {
            Server_RemovePatrolRouteForUnit(Unit);
			NewRoute.AssignedUnits.AddUnique(Unit);
        }
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

            if (Route.PatrolID == UISelectedPatrolID)
            {
                bIsRouteVisible = true;
            }
            else if (SelectionComponent)
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
	Extended.PatrolID = Route.PatrolID;
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
		if (IPatrolAgentInterface* AI = Cast<IPatrolAgentInterface>(Pawn->GetController()))
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
	else if (IPatrolAgentInterface* Agent = Cast<IPatrolAgentInterface>(Unit))
	{
		int32 CurrentIndex = Agent->GetCurrentPatrolWaypointIndex();

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

	return 0;
}


void UUnitPatrolComponent::NotifyPatrolUpdate(const FPatrolRoute& Route, bool bIsReverse)
{
    if (!HasAuthority())
        return;

    const bool bLoop = (Route.PatrolType == EPatrolType::Loop);
    const int32 NumPoints = Route.PatrolPoints.Num();

    for (const auto& UnitPtr : Route.AssignedUnits)
    {
        AActor* Unit = UnitPtr.Get();
        if (APawn* Pawn = Cast<APawn>(Unit))
        {
            if (IPatrolAgentInterface* AI = Cast<IPatrolAgentInterface>(Pawn->GetController()))
            {
                int32 NewStartIndex = GetTargetPointIndexForUnit(Unit, NumPoints, bIsReverse);
                AI->UpdatePatrolRoute(Route.PatrolPoints, bLoop, NewStartIndex);
            }
            else if (IPatrolAgentInterface* Agent = Cast<IPatrolAgentInterface>(Unit))
            {
				int32 NewStartIndex = GetTargetPointIndexForUnit(Unit, NumPoints, bIsReverse);
                Agent->UpdatePatrolRoute(Route.PatrolPoints, bLoop, NewStartIndex);
            }
        }
		else if (IPatrolAgentInterface* Agent = Cast<IPatrolAgentInterface>(Unit))
		{
			int32 NewStartIndex = GetTargetPointIndexForUnit(Unit, NumPoints, bIsReverse);
			Agent->UpdatePatrolRoute(Route.PatrolPoints, bLoop, NewStartIndex);
		}
    }
}

void UUnitPatrolComponent::SetUISelectedPatrol(FGuid PatrolID)
{
    if (UISelectedPatrolID != PatrolID)
    {
        UISelectedPatrolID = PatrolID;
        if (IsLocallyControlled())
        {
            UpdateVisualization();
            OnPatrolSelected.Broadcast(UISelectedPatrolID);
        }
    }
}