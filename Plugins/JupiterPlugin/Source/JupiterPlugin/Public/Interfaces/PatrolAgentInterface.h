#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PatrolAgentInterface.generated.h"


UINTERFACE(MinimalAPI)
class UPatrolAgentInterface : public UInterface
{
	GENERATED_BODY()
};

class JUPITERPLUGIN_API IPatrolAgentInterface
{
	GENERATED_BODY()

public:
	virtual void UpdatePatrolRoute(const TArray<FVector>& Points, bool bLoop, int32 StartIndex) = 0;

	virtual void StopPatrol() = 0;

	virtual void PausePatrol(bool bPause) = 0;

	virtual int32 GetCurrentPatrolWaypointIndex() const = 0;
};
