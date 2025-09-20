#include "Player/Selections/SphereRadius.h"
#include "Player/PlayerControllerRts.h"
#include "Components/DecalComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

ASphereRadius::ASphereRadius()
{
	PrimaryActorTick.bCanEverTick = true;

	
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->SetSphereRadius(1.f);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = SphereComponent;
	
	Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
	Decal->SetupAttachment(RootComponent);
	
	SphereEnable = false;
}

void ASphereRadius::BeginPlay()
{
	Super::BeginPlay();
	
	if(Decal) Decal->SetVisibility(false);
	
	SetActorEnableCollision(false);
        if (APawn* OwnerPawn = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetPawn())
        {
                SelectionComponent = OwnerPawn->FindComponentByClass<UUnitSelectionComponent>();
        }
}

void ASphereRadius::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if(SphereEnable)
		CurrentRadius = Adjust();
}

float ASphereRadius::Adjust() const
{
	if (!SelectionComponent || !SphereComponent || !Decal) return 0.f;

	const FVector CurrentMouseLocOnTerrain = SelectionComponent->GetMousePositionOnTerrain().Location;
	const FVector EndPoint = FVector(CurrentMouseLocOnTerrain.X, CurrentMouseLocOnTerrain.Y, 0.0f);

	FVector NewLocation = UKismetMathLibrary::VLerp(StartLocation, EndPoint, 0.5f);
	SphereComponent->SetWorldLocation(NewLocation);

	float NewScale = FVector::Distance(FVector(NewLocation.X, NewLocation.Y, 0.0f), EndPoint);
	//SphereComponent->SetSphereRadius(NewScale);

	FVector DecalSize = FVector(NewScale * 2.0f, NewScale * 2.0f, NewScale * 2.0f);
	Decal->DecalSize = DecalSize;

	return NewScale;
}

void ASphereRadius::Start(FVector Position, const FRotator Rotation)
{
	if(!Decal) return;

	StartLocation = FVector(Position.X, Position.Y, 0.0f);
	StartRotation = FRotator(0.f, Rotation.Yaw, 0.0f);

	SetActorLocation(StartLocation);
	SetActorRotation(StartRotation);
	SetActorEnableCollision(true);
	
	Decal->SetVisibility(true);
	SphereEnable = true;
}

void ASphereRadius::End()
{
	if(!Decal) return;

	SphereEnable = false;
	SetActorEnableCollision(false);
	Decal->SetVisibility(false);
}

float ASphereRadius::GetRadius() const
{
	return CurrentRadius;
}

