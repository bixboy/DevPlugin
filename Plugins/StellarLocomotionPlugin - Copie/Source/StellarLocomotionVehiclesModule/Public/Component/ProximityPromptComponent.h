#pragma once
#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "ProximityPromptComponent.generated.h"

struct FEnhancedInputActionEventBinding;
class UProximityPromptComponent;
class UPromptWidget;
class UInputAction;



UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STELLARLOCOMOTIONVEHICLESMODULE_API UProximityPromptComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UProximityPromptComponent();
	
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	
	// Getters
	
	UFUNCTION(BlueprintPure, Category="ProximityPrompt")
	float GetActivationDistance() const { return ActivationDistance; }

	UFUNCTION(BlueprintPure, Category="ProximityPrompt")
	FVector GetPromptLocation() const { return GetComponentLocation(); }

	UFUNCTION(BlueprintPure, Category="ProximityPrompt")
	bool IsPromptEnabled() const { return bIsEnabled; }

	UFUNCTION(BlueprintPure, Category="ProximityPrompt")
	bool IsPromptVisible() const { return bIsVisible; }
	

	// Functions
	
	UFUNCTION(BlueprintCallable, Category="ProximityPrompt")
	void SetPromptVisible(bool bNewVisible);

	UFUNCTION(BlueprintCallable, Category="ProximityPrompt")
	void SetPromptEnabled(bool bNewEnabled);

	UPROPERTY(EditAnywhere, Category="Prompt")
	UInputAction* InteractInputAction;

protected:

	UFUNCTION()
	void TryRegisterPrompt();

	//Settings
	
	UPROPERTY(EditAnywhere, Category="Prompt")
	float ActivationDistance = 300.f;

	UPROPERTY(EditAnywhere, Category="Prompt")
	float WidgetScale = 0.3f;

	UPROPERTY(EditAnywhere, Category="Prompt")
	TSubclassOf<UUserWidget> PromptWidgetClass;

	
	//Ref
	
	UPROPERTY()
	UWidgetComponent* PromptWidgetComponent = nullptr;

	UPROPERTY()
	UPromptWidget* PromptWidget = nullptr;

	UPROPERTY()
	APlayerController* LocalPC = nullptr;

	UPROPERTY()
	bool bIsVisible = false;

	UPROPERTY()
	bool bIsEnabled = true;

	
	// Text Update
	
	UFUNCTION()
	TArray<FKey> GetKeysForAction(UInputAction* Action) const;

	UFUNCTION()
	void UpdatePromptText();
};
