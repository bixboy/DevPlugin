#include "Player/Selections/SphereRadius.h"

#include "Components/DecalComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
ASphereRadius::ASphereRadius()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root collision volume
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(1.f);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = SphereComponent;

	// Visual decal
	DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
	DecalComponent->SetupAttachment(RootComponent);
	DecalComponent->SetVisibility(false);

	bActive = false;
}


// ------------------------------------------------------------
// BeginPlay
// ------------------------------------------------------------
void ASphereRadius::BeginPlay()
{
	Super::BeginPlay();

	SetActorEnableCollision(false);

	// Cache selection component (faster than searching every tick)
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			SelectionComponent = Pawn->FindComponentByClass<UUnitSelectionComponent>();
		}
	}
}


// ------------------------------------------------------------
// Tick
// ------------------------------------------------------------
void ASphereRadius::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bActive)
	{
		CurrentRadius = ComputeRadius();
	}
}


// ------------------------------------------------------------
// Compute radius (core behaviour)
// ------------------------------------------------------------
float ASphereRadius::ComputeRadius()
{
	if (!SelectionComponent || !DecalComponent)
		return 0.f;

	// Mouse → terrain hit
	const FHitResult Hit = SelectionComponent->GetMousePositionOnTerrain();
	const FVector EndPoint(Hit.Location.X, Hit.Location.Y, 0.f);

	// Mid-point for correct ground positioning of the decal
	const FVector NewLocation = (StartLocation + EndPoint) * 0.5f;
	SetActorLocation(NewLocation);

	// 2D distance → sphere radius
	const FVector A2D(NewLocation.X, NewLocation.Y, 0.f);
	const FVector B2D(EndPoint.X, EndPoint.Y, 0.f);
	const float Radius = FVector::Distance(A2D, B2D);

	// Update decal scale
	DecalComponent->DecalSize = FVector(Radius * 2.f, Radius * 2.f, Radius * 2.f);

	return Radius;
}


// ------------------------------------------------------------
// Start preview
// ------------------------------------------------------------
void ASphereRadius::Start(FVector Position, FRotator Rotation)
{
	if (!DecalComponent)
		return;

	StartLocation = FVector(Position.X, Position.Y, 0.f);
	StartRotation = FRotator(0.f, Rotation.Yaw, 0.f);

	SetActorLocation(StartLocation);
	SetActorRotation(StartRotation);

	SetActorEnableCollision(true);
	DecalComponent->SetVisibility(true);

	bActive = true;
}


// ------------------------------------------------------------
// End preview
// ------------------------------------------------------------
void ASphereRadius::End()
{
	if (!DecalComponent)
		return;

	bActive = false;

	SetActorEnableCollision(false);
	DecalComponent->SetVisibility(false);
}
