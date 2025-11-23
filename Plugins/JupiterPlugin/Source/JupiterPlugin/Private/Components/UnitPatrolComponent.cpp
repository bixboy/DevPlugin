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
	if (OrderComponent)
	{
		OrderComponent->OnOrdersDispatched.RemoveDynamic(this, &UUnitPatrolComponent::HandleOrdersDispatched);
		OrderComponent = nullptr;
	}

	if (SelectionComponent)
	{
		SelectionComponent->OnSelectionChanged.RemoveDynamic(this, &UUnitPatrolComponent::OnSelectionChanged);
		SelectionComponent = nullptr;
	}

	if (PatrolVisualizer)
	{
		PatrolVisualizer->DestroyComponent();
		PatrolVisualizer = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UUnitPatrolComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsLocallyControlled())
		return;

	if (bAutoCreateVisualizer)
	{
		EnsureVisualizerComponent();
	}
}

void UUnitPatrolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UUnitPatrolComponent, ActivePatrolRoutes);
}

// ============================================================
// INTERNAL HELPERS
// ============================================================

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
	Extended.bIsLoop = Route.bIsLoop;
	Extended.RouteColor = Color;
	Extended.bShowDirectionArrows = true;
	Extended.bShowWaypointNumbers = true;
	return Extended;
}

bool UUnitPatrolComponent::IsLocallyControlled() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

bool UUnitPatrolComponent::HasAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

// ============================================================
// EVENT HANDLERS
// ============================================================

void UUnitPatrolComponent::HandleOrdersDispatched(const TArray<AActor*>& AffectedUnits, const FCommandData& IssuedCommand)
{
	if (IssuedCommand.Type != ECommandType::CommandPatrol)
	{
		if (!ActivePatrolRoutes.IsEmpty())
		{
			TArray<FPatrolRoute> EmptyRoutes;
			ApplyRoutes(EmptyRoutes);
		}
		return;
	}

	TArray<FPatrolRoute> NewRoutes;
	NewRoutes.Reserve(AffectedUnits.Num());

	for (AActor* Unit : AffectedUnits)
	{
		if (!Unit || !Unit->Implements<USelectable>())
			continue;

		const FCommandData CurrentCommand = ISelectable::Execute_GetCurrentCommand(Unit);
		if (CurrentCommand.Type != ECommandType::CommandPatrol || CurrentCommand.PatrolPath.Num() < 2)
			continue;

		FPatrolRoute Route;
		Route.PatrolPoints = CurrentCommand.PatrolPath;
		Route.bIsLoop = CurrentCommand.bPatrolLoop;
		NewRoutes.Add(Route);
	}

	if (NewRoutes.IsEmpty() && IssuedCommand.PatrolPath.Num() >= 2)
	{
		FPatrolRoute Route;
		Route.PatrolPoints = IssuedCommand.PatrolPath;
		// NOTE: If the user reports "Loop persistence" issues, check the Input/UI logic that creates 'IssuedCommand'.
		// The 'bPatrolLoop' flag might not be resetting correctly in the PlayerController or HUD between commands.
		Route.bIsLoop = IssuedCommand.bPatrolLoop;
		NewRoutes.Add(Route);
	}

	ApplyRoutes(NewRoutes);
}

void UUnitPatrolComponent::OnSelectionChanged(const TArray<AActor*>& SelectedActors)
{
	RefreshRoutesFromSelection();
}

void UUnitPatrolComponent::RefreshRoutesFromSelection()
{
	if (!SelectionComponent)
	{
		if (AActor* OwnerActor = GetOwner())
		{
			SelectionComponent = OwnerActor->FindComponentByClass<UUnitSelectionComponent>();
		}
	}

	if (!SelectionComponent)
		return;

	const TArray<AActor*> SelectedActors = SelectionComponent->GetSelectedActors();

	TArray<FPatrolRoute> NewRoutes;
	NewRoutes.Reserve(SelectedActors.Num());

	for (AActor* Unit : SelectedActors)
	{
		if (!Unit || !Unit->Implements<USelectable>())
			continue;

		const FCommandData CurrentCommand = ISelectable::Execute_GetCurrentCommand(Unit);
		if (CurrentCommand.Type != ECommandType::CommandPatrol || CurrentCommand.PatrolPath.Num() < 2)
			continue;

		FPatrolRoute Route;
		Route.PatrolPoints = CurrentCommand.PatrolPath;
		Route.bIsLoop = CurrentCommand.bPatrolLoop;
		NewRoutes.Add(Route);
	}

	ApplyRoutes(NewRoutes);
}

namespace
{
	bool AreRoutesEquivalent(const TArray<FPatrolRoute>& A, const TArray<FPatrolRoute>& B)
	{
		if (A.Num() != B.Num())
			return false;

		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			const FPatrolRoute& RouteA = A[Index];
			const FPatrolRoute& RouteB = B[Index];

			if (RouteA.bIsLoop != RouteB.bIsLoop || RouteA.PatrolPoints.Num() != RouteB.PatrolPoints.Num())
				return false;

			for (int32 PointIdx = 0; PointIdx < RouteA.PatrolPoints.Num(); ++PointIdx)
			{
				if (!RouteA.PatrolPoints[PointIdx].Equals(RouteB.PatrolPoints[PointIdx], KINDA_SMALL_NUMBER))
					return false;
			}
		}

		return true;
	}
}

void UUnitPatrolComponent::ApplyRoutes(const TArray<FPatrolRoute>& NewRoutes)
{
	if (AreRoutesEquivalent(ActivePatrolRoutes, NewRoutes))
		return;

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
