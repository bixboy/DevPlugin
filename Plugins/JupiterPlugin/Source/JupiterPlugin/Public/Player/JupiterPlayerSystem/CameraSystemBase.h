#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CameraSystemBase.generated.h"

class APlayerCamera;
class UUnitSelectionComponent;
class UUnitOrderComponent;
class UUnitFormationComponent;
class UPlacementHandlerComponent;
class UUnitPatrolComponent;



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
	
	UPlacementHandlerComponent* GetPlacementComponent() const;
	
	UUnitPatrolComponent* GetPatrolComponent() const;

	UWorld* GetWorldSafe() const;
};
