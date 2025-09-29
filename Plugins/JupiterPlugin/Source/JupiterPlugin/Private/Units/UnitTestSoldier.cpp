#include "Units/UnitTestSoldier.h"

#include "DynamicPathfindingRuntime/Public/DynamicPathManager.h"
#include "DynamicPathfindingRuntime/Public/UnitAvoidanceComponent.h"
#include "DynamicPathfindingRuntime/Public/UnitNavigationComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/UnrealType.h"

AUnitTestSoldier::AUnitTestSoldier()
{
    UnitAvoidanceComponent = CreateDefaultSubobject<UUnitAvoidanceComponent>(TEXT("UnitAvoidanceComponent"));
    UnitNavigationComponent = CreateDefaultSubobject<UUnitNavigationComponent>(TEXT("UnitNavigationComponent"));

    InitializeNavigationDefaults();
}

void AUnitTestSoldier::BeginPlay()
{
    Super::BeginPlay();

    InitializeNavigationDefaults();
}

void AUnitTestSoldier::CommandMove_Implementation(FCommandData CommandData)
{
    Super::CommandMove_Implementation(CommandData);

    if (UnitNavigationComponent)
    {
        UnitNavigationComponent->EnsureManagerReference();

        if (ADynamicPathManager* Manager = UnitNavigationComponent->GetManager())
        {
            const FVector TargetLocation = CommandData.Target && IsValid(CommandData.Target)
                                              ? CommandData.Target->GetActorLocation()
                                              : CommandData.Location;
            Manager->SetTargetLocation(TargetLocation);
        }
    }
}

#if WITH_EDITOR
void AUnitTestSoldier::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    InitializeNavigationDefaults();
}
#endif

void AUnitTestSoldier::InitializeNavigationDefaults()
{
    if (UnitNavigationComponent)
    {
        UnitNavigationComponent->EnsureManagerReference();

        if (const UCharacterMovementComponent* Movement = GetCharacterMovement())
        {
            if (UnitNavigationComponent->MoveSpeed <= 0.f)
            {
                UnitNavigationComponent->MoveSpeed = Movement->MaxWalkSpeed;
            }

            if (UnitNavigationComponent->MaxSpeed <= 0.f)
            {
                UnitNavigationComponent->MaxSpeed = Movement->GetMaxSpeed();
            }
        }
    }

    if (UnitAvoidanceComponent && UnitNavigationComponent)
    {
        if (ADynamicPathManager* Manager = UnitNavigationComponent->GetManager())
        {
            UnitAvoidanceComponent->SetManager(Manager);
        }
    }
}
