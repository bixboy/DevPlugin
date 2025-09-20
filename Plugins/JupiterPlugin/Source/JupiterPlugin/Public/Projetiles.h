#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projetiles.generated.h"

UCLASS()
class JUPITERPLUGIN_API AProjetiles : public AActor
{
	GENERATED_BODY()

public:
	AProjetiles();

	UFUNCTION()
	void SetOwnerActor(AActor* NewOwner);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly)
	AActor* OwnerActor;
};
