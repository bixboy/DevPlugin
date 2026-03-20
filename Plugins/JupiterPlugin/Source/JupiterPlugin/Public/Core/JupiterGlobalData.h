#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "JupiterGlobalData.generated.h"

class UPresetManagerComponent;
class UUnitPatrolComponent;


UCLASS()
class JUPITERPLUGIN_API AJupiterGlobalData : public AInfo
{
	GENERATED_BODY()
	
public:
	AJupiterGlobalData();

	UFUNCTION(BlueprintPure, Category = "Jupiter|Global", meta = (WorldContext = "WorldContextObject"))
	static AJupiterGlobalData* Get(const UObject* WorldContextObject);

	static AJupiterGlobalData* EnsureExists(UWorld* World);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPresetManagerComponent> PresetManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUnitPatrolComponent> PatrolComponent;

public:
	FORCEINLINE UPresetManagerComponent* GetPresetManager() const { return PresetManager; }
	FORCEINLINE UUnitPatrolComponent* GetPatrolComponent() const { return PatrolComponent; }
};
