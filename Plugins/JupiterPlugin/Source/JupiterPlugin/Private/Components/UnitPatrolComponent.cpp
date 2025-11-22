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

	// Find dependencies
	SelectionComponent = OwnerActor->FindComponentByClass<UUnitSelectionComponent>();
	OrderComponent = OwnerActor->FindComponentByClass<UUnitOrderComponent>();

	// Subscribe to order events
	if (OrderComponent)
	{
		OrderComponent->OnOrdersDispatched.AddDynamic(this, &UUnitPatrolComponent::HandleOrdersDispatched);
	}

	// Create visualizer if needed and on client
	if (IsLocallyControlled() && bAutoCreateVisualizer)
	{
		EnsureVisualizerComponent();
	}
}

void UUnitPatrolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unsubscribe from events
	if (OrderComponent)
	{
		OrderComponent->OnOrdersDispatched.RemoveDynamic(this, &UUnitPatrolComponent::HandleOrdersDispatched);
		OrderComponent = nullptr;
	}

	// Clean up visualizer
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

	// Only tick on local client for visualization
	if (!IsLocallyControlled())
		return;

	// Ensure visualizer exists
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
// INPUT HANDLING
// ============================================================

bool UUnitPatrolComponent::HandleLeftClick(EInputEvent InputEvent)
{
	if (!IsLocallyControlled())
		return false;

	if (InputEvent == IE_Released)
	{
		RefreshRoutesFromSelection();
	}

	return false;
}

bool UUnitPatrolComponent::HandleRightClickPressed(bool /*bAltDown*/)
{
	return false;
}

bool UUnitPatrolComponent::HandleRightClickReleased(bool /*bAltDown*/)
{
	if (!IsLocallyControlled())
		return false;

	RefreshRoutesFromSelection();
	return false;
}

// ============================================================
// PATROL PREVIEW SYSTEM
// ============================================================

void UUnitPatrolComponent::StartPatrolPreview(const FVector& StartPoint)
{
	if (!IsLocallyControlled())
		return;

	bIsCreatingPreview = true;
	
	PreviewRoute = FPatrolRouteExtended();
	PreviewRoute.PatrolPoints.Add(StartPoint);
	PreviewRoute.RouteColor = FLinearColor(1.0f, 1.0f, 0.0f, 0.6f); // Yellow preview

	UpdateVisualization();
}

void UUnitPatrolComponent::AddPreviewWaypoint(const FVector& Point)
{
	if (!IsLocallyControlled() || !bIsCreatingPreview)
		return;

	PreviewRoute.PatrolPoints.Add(Point);
	UpdateVisualization();
}

void UUnitPatrolComponent::UpdatePreviewCursor(const FVector& CursorPosition)
{
	if (!IsLocallyControlled() || !bIsCreatingPreview)
		return;

	// Update the last point to follow cursor (for smooth preview)
	if (PreviewRoute.PatrolPoints.Num() > 0)
	{
		PreviewRoute.PatrolPoints.Last() = CursorPosition;
		UpdateVisualization();
	}
}

void UUnitPatrolComponent::CommitPatrolPreview(bool bLoop)
{
	if (!IsLocallyControlled() || !bIsCreatingPreview)
		return;

	// Convert preview to actual route
	FPatrolRoute NewRoute;
	NewRoute.PatrolPoints = PreviewRoute.PatrolPoints;
	NewRoute.bIsLoop = bLoop;

	// Apply the route
	TArray<FPatrolRoute> Routes;
	Routes.Add(NewRoute);
	ApplyRoutes(Routes);

	// Clear preview state
	bIsCreatingPreview = false;
	PreviewRoute = FPatrolRouteExtended();
	
	UpdateVisualization();
}

void UUnitPatrolComponent::CancelPatrolPreview()
{
	if (!IsLocallyControlled())
		return;

	bIsCreatingPreview = false;
	PreviewRoute = FPatrolRouteExtended();
	
	UpdateVisualization();
}

// ============================================================
// PATROL MANAGEMENT
// ============================================================

void UUnitPatrolComponent::SetPatrolRoutes(const TArray<FPatrolRoute>& NewRoutes)
{
	ApplyRoutes(NewRoutes);
}

void UUnitPatrolComponent::ClearAllRoutes()
{
	TArray<FPatrolRoute> EmptyRoutes;
	ApplyRoutes(EmptyRoutes);
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

	// Check if one already exists
	PatrolVisualizer = Owner->FindComponentByClass<UPatrolVisualizerComponent>();
	
	if (!PatrolVisualizer)
	{
		// Create new visualizer component
		PatrolVisualizer = NewObject<UPatrolVisualizerComponent>(Owner, UPatrolVisualizerComponent::StaticClass(), TEXT("PatrolVisualizer"));
		if (PatrolVisualizer)
		{
			PatrolVisualizer->RegisterComponent();
			UpdateVisualization(); // Initial update
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

	// Build extended routes for visualization
	TArray<FPatrolRouteExtended> ExtendedRoutes;

	// Add preview route if in creation mode
	if (bIsCreatingPreview && PreviewRoute.PatrolPoints.Num() >= 2)
	{
		ExtendedRoutes.Add(PreviewRoute);
	}

	// Add active routes
	for (const FPatrolRoute& Route : ActivePatrolRoutes)
	{
		if (Route.PatrolPoints.Num() >= 2)
		{
			ExtendedRoutes.Add(ConvertToExtended(Route, RouteColor));
		}
	}

	// Update visualizer
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
	// Clear routes if non-patrol command
	if (IssuedCommand.Type != ECommandType::CommandPatrol)
	{
		if (!ActivePatrolRoutes.IsEmpty())
		{
			TArray<FPatrolRoute> EmptyRoutes;
			ApplyRoutes(EmptyRoutes);
		}
		return;
	}

	// Build new routes from affected units
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

	// Fallback: use issued command if no valid routes from units
	if (NewRoutes.IsEmpty() && IssuedCommand.PatrolPath.Num() >= 2)
	{
		FPatrolRoute Route;
		Route.PatrolPoints = IssuedCommand.PatrolPath;
		Route.bIsLoop = IssuedCommand.bPatrolLoop;
		NewRoutes.Add(Route);
	}

	ApplyRoutes(NewRoutes);
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
	// Avoid unnecessary updates
	if (AreRoutesEquivalent(ActivePatrolRoutes, NewRoutes))
		return;

	ActivePatrolRoutes = NewRoutes;
	
	// Update visualization on local client
	if (IsLocallyControlled())
	{
		UpdateVisualization();
	}
}

void UUnitPatrolComponent::OnRep_ActivePatrolRoutes()
{
	// Routes replicated from server - update visualization
	if (IsLocallyControlled())
	{
		UpdateVisualization();
	}
}
