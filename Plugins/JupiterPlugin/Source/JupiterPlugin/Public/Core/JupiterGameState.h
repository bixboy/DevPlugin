#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "JupiterGameState.generated.h"

class USoldierManagerComponent;


UCLASS()
class JUPITERPLUGIN_API AJupiterGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AJupiterGameState();

	UFUNCTION(BlueprintCallable, Category="RTS")
	USoldierManagerComponent* GetSoldierManager() const { return SoldierManager; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RTS", meta=(AllowPrivateAccess="true"))
	USoldierManagerComponent* SoldierManager;
};
