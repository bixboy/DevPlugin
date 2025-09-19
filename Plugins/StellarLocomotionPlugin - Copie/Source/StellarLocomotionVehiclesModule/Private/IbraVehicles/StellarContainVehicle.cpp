// Fill out your copyright notice in the Description page of Project Settings.


#include "IbraVehicles/StellarContainVehicle.h"

#include "Components/BoxComponent.h"


// Sets default values
AStellarContainVehicle::AStellarContainVehicle()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VehicleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VehicleRoot"));
	RootComponent = VehicleRoot;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	
	TriggerBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
}

// Called when the game starts or when spawned
void AStellarContainVehicle::BeginPlay()
{
	Super::BeginPlay();


	// FActorSpawnParameters SpawnParams;
	// SpawnParams.Owner = this;
	// SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	//
	// FVector SpawnLocation = GetActorLocation() + FVector(0,0,1000); // or any location
	// FRotator SpawnRotation = FRotator::ZeroRotator;
	//
	// UClass* ClassToUse = GetClass();
	//
	// if (ClassToUse->IsChildOf(AStellarContainVehicle::StaticClass()))
	// {
	// 	AActor* Instance = GetWorld()->SpawnActor<AActor>(
	// 		ClassToUse,
	// 		SpawnLocation,
	// 		SpawnRotation,
	// 		SpawnParams
	// 	);
	//
	// 	CollisionInstance = Cast<AStellarContainVehicle>(Instance);
	// }
	// else
	// {
	// }
	
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AStellarContainVehicle::OnOverlapBegin);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AStellarContainVehicle::OnOverlapEnd);
}

// Called every frame
void AStellarContainVehicle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AStellarContainVehicle::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(!HasAuthority()) return;

	if(AStellarControllableClass* OverlappingObj = Cast<AStellarControllableClass>(OtherActor))
	{
		OverlappingObj->SetEnvironment(this);
	}
}

void AStellarContainVehicle::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(!HasAuthority()) return;

	if(AStellarControllableClass* OverlappingObj = Cast<AStellarControllableClass>(OtherActor))
	{
		OverlappingObj->SetEnvironment(nullptr);
	}
}

