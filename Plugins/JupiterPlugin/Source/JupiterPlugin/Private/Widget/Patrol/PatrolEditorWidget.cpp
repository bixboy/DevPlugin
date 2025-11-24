#include "Widget/Patrol/PatrolEditorWidget.h"
#include "Widget/Patrol/PatrolRouteRowWidget.h"
#include "Components/UnitPatrolComponent.h"
#include "Components/ScrollBox.h"
#include "Interfaces/Selectable.h"
#include "Kismet/GameplayStatics.h"

void UPatrolEditorWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UPatrolEditorWidget::SetupWithComponent(UUnitPatrolComponent* InComponent)
{
	UE_LOG(LogTemp, Log, TEXT("[PatrolEditor] SetupWithComponent called, Component: %s"), InComponent ? TEXT("Valid") : TEXT("NULL"));
	PatrolComponent = InComponent;
	bIsGlobalMode = false;
	Refresh();
}

void UPatrolEditorWidget::SetupGlobal()
{
	UE_LOG(LogTemp, Log, TEXT("[PatrolEditor] SetupGlobal called"));
	PatrolComponent = nullptr;
	bIsGlobalMode = true;
	Refresh();
}

void UPatrolEditorWidget::Refresh()
{
	UE_LOG(LogTemp, Log, TEXT("[PatrolEditor] Refresh called"));
	
	if (!RouteList)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PatrolEditor] RouteList is NULL!"));
		return;
	}

	RouteList->ClearChildren();

	if (!RouteRowClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[PatrolEditor] RouteRowClass is NOT SET! Please assign it in the Blueprint!"));
		return;
	}

	if (bIsGlobalMode)
	{
		// Global mode: Display all patrol routes from the manager
		
		// Ensure we have a PatrolComponent (Manager)
		if (!PatrolComponent)
		{
			if (APlayerController* PC = GetOwningPlayer())
			{
				if (APawn* PlayerPawn = PC->GetPawn())
				{
					PatrolComponent = PlayerPawn->FindComponentByClass<UUnitPatrolComponent>();
				}
			}
		}
		
		if (!PatrolComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PatrolEditor] Global Mode: Could not find UUnitPatrolComponent on PlayerPawn!"));
			return;
		}

		// Use the grouped routes from the component instead of finding units individually
		const TArray<FPatrolRoute>& Routes = PatrolComponent->GetActiveRoutes();
		UE_LOG(LogTemp, Log, TEXT("[PatrolEditor] Global Mode: Number of grouped routes: %d"), Routes.Num());
		
		for (int32 i = 0; i < Routes.Num(); ++i)
		{
			UPatrolRouteRowWidget* Row = CreateWidget<UPatrolRouteRowWidget>(this, RouteRowClass);
			if (Row)
			{
				Row->Setup(PatrolComponent, i);
				RouteList->AddChild(Row);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[PatrolEditor] Global Refresh complete, total routes: %d"), Routes.Num());
	}
	else
	{
		// Single component mode
		if (!PatrolComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PatrolEditor] PatrolComponent is NULL!"));
			return;
		}

		const TArray<FPatrolRoute>& Routes = PatrolComponent->GetActiveRoutes();
		UE_LOG(LogTemp, Log, TEXT("[PatrolEditor] Number of routes: %d"), Routes.Num());
		
		for (int32 i = 0; i < Routes.Num(); ++i)
		{
			UE_LOG(LogTemp, Log, TEXT("[PatrolEditor] Creating row widget %d/%d"), i+1, Routes.Num());
			UPatrolRouteRowWidget* Row = CreateWidget<UPatrolRouteRowWidget>(this, RouteRowClass);
			if (Row)
			{
				Row->Setup(PatrolComponent, i);
				RouteList->AddChild(Row);
				UE_LOG(LogTemp, Log, TEXT("[PatrolEditor] Row widget %d added successfully"), i);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[PatrolEditor] Failed to create row widget %d"), i);
			}
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("[PatrolEditor] Refresh complete, total children: %d"), RouteList->GetChildrenCount());
}
