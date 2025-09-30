#include "FlowFieldSampleActors.h"
#include "DynamicFlowFieldManager.h"
#include "UnitFlowComponent.h"
#include "LocalAvoidanceComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"

AFlowFieldSampleUnit::AFlowFieldSampleUnit()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetSimulatePhysics(false);
    Mesh->SetEnableGravity(false);

    FlowComponent = CreateDefaultSubobject<UUnitFlowComponent>(TEXT("FlowComponent"));
    AvoidanceComponent = CreateDefaultSubobject<ULocalAvoidanceComponent>(TEXT("AvoidanceComponent"));
}

void AFlowFieldSampleUnit::BeginPlay()
{
    Super::BeginPlay();
}

void AFlowFieldSampleUnit::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const FVector FlowDir = FlowComponent ? FlowComponent->SampleDirection() : FVector::ZeroVector;
    const FVector Avoidance = AvoidanceComponent ? AvoidanceComponent->GetAvoidanceVector() : FVector::ZeroVector;
    FVector Desired = FlowDir + Avoidance;
    if (!Desired.IsNearlyZero())
    {
        Desired = Desired.GetSafeNormal();
    }

    const FVector Velocity = Desired * MoveSpeed;
    AddActorWorldOffset(Velocity * DeltaSeconds, true);
    if (!Velocity.IsNearlyZero())
    {
        const FRotator DesiredRot = Velocity.Rotation();
        SetActorRotation(DesiredRot);
    }
}

AFlowFieldSampleManagerActor::AFlowFieldSampleManagerActor()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
}

void AFlowFieldSampleManagerActor::BeginPlay()
{
    Super::BeginPlay();

    if (!Manager)
    {
        Manager = GetWorld()->SpawnActor<ADynamicFlowFieldManager>();
    }

    if (Manager)
    {
        Manager->SetGlobalTarget(GetActorLocation());
    }

    if (InitialUnits > 0)
    {
        SpawnUnits(InitialUnits);
    }
}

void AFlowFieldSampleManagerActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

void AFlowFieldSampleManagerActor::SpawnUnits(int32 Count)
{
    if (!UnitClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("No UnitClass configured for FlowFieldSampleManagerActor"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (int32 i = 0; i < Count; ++i)
    {
        const FVector SpawnLocation = GetActorLocation() + UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f, SpawnRadius);
        const FRotator SpawnRotation = FRotator::ZeroRotator;
        AFlowFieldSampleUnit* Unit = World->SpawnActor<AFlowFieldSampleUnit>(UnitClass, SpawnLocation, SpawnRotation);
        if (Unit)
        {
            if (Unit->FlowComponent)
            {
                Unit->FlowComponent->FlowFieldManager = Manager;
            }
            SpawnedUnits.Add(Unit);
        }
    }
}

void AFlowFieldSampleManagerActor::SetTargetAtLocation(const FVector& Location)
{
    if (Manager)
    {
        Manager->SetGlobalTarget(Location);
    }
}

void AFlowFieldSampleManagerActor::ToggleDebug()
{
    if (Manager)
    {
        Manager->SetDebugEnabled(!Manager->IsDebugEnabled());
    }
}

