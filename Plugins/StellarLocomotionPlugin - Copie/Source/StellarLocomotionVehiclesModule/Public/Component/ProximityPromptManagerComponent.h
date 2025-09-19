#pragma once
#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "Components/ActorComponent.h"
#include "ProximityPromptManagerComponent.generated.h"

class UInputAction;
class UProximityPromptComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPromptInteracted, UProximityPromptComponent*, ProximityPrompt, APlayerController*, InteractingPC, UObject*, ContextObject);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STELLARLOCOMOTIONVEHICLESMODULE_API UProximityPromptManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UProximityPromptManagerComponent();
	
	virtual void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void RegisterPrompt(UProximityPromptComponent* Prompt);

	UFUNCTION(Server, Reliable)
	void Server_RegisterPrompt(UProximityPromptComponent* Prompt);

	UFUNCTION()
	void SetAllPromptsEnabled(bool bNewEnabled);

	UPROPERTY(BlueprintAssignable)
	FOnPromptInteracted OnPromptInteracted;

protected:

	UPROPERTY()
	TWeakObjectPtr<UProximityPromptComponent> CurrentPrompt;

	UPROPERTY()
	TArray<UProximityPromptComponent*> AllPrompts;

	UPROPERTY()
	AActor* Player;

	UPROPERTY()
	APlayerController* PC;

	UPROPERTY()
	TSet<TObjectPtr<UInputAction>> BoundActions;

	UFUNCTION()
	void OnAnyPromptAction(const FInputActionInstance& Instance);

	UFUNCTION(Server, Reliable)
	void Server_NotifyBestPrompt(AActor* PromptOwner, int32 PromptIndex);
	
	UFUNCTION(Client, Reliable)
	void Client_SetPrompt(AActor* PromptOwner, int32 PromptIndex);
	
};
