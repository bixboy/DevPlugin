#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CameraSystemBase.generated.h"

class APlayerCamera;
class UUnitSelectionComponent;
class UUnitOrderComponent;
class UUnitFormationComponent;
class UUnitSpawnComponent;
class UUnitPatrolComponent;


/**
 * Base class for all PlayerCamera systems.
 */
UCLASS(Abstract)
class JUPITERPLUGIN_API UCameraSystemBase : public UObject
{
	GENERATED_BODY()

public:

	virtual void Init(APlayerCamera* InOwner);

	virtual void Tick(float DeltaTime);

protected:

	TWeakObjectPtr<APlayerCamera> Owner;

	APlayerCamera* GetOwner() const;

	UUnitSelectionComponent* GetSelectionComponent() const;
	UUnitOrderComponent* GetOrderComponent() const;
	UUnitFormationComponent* GetFormationComponent() const;
	UUnitSpawnComponent* GetSpawnComponent() const;
	UUnitPatrolComponent* GetPatrolComponent() const;

	UWorld* GetWorldSafe() const;
};
