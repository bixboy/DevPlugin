#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerControllerRts.generated.h"

class UUnitSelectionComponent;
class UInputMappingContext;


UCLASS()
class JUPITERPLUGIN_API APlayerControllerRts : public APlayerController
{
	GENERATED_BODY()

public:
	APlayerControllerRts(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        UUnitSelectionComponent* SelectionComponent;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;
	
};
