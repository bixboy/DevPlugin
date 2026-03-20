#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Placement/PresetData.h"
#include "PresetManagerComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPresetsChanged);

UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UPresetManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPresetManagerComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool IsGlobalManager() const;

	UFUNCTION()
	void OnGlobalPresetsChanged();

	UFUNCTION()
	void OnRep_SavedPresets();

	void BindToGlobalData();


	UFUNCTION(BlueprintCallable, Category = "Jupiter|Presets")
	FGuid CreatePresetFromActors(const TArray<AActor*>& Actors, FName PresetName);

	UFUNCTION(BlueprintCallable, Category = "Jupiter|Presets")
	void DeletePreset(FGuid PresetID);

	UFUNCTION(BlueprintCallable, Category = "Jupiter|Presets")
	void RenamePreset(FGuid PresetID, FName NewName);

	UFUNCTION(BlueprintPure, Category = "Jupiter|Presets")
	const TArray<FPlacementPreset>& GetAllPresets() const;

	const FPlacementPreset* FindPreset(FGuid PresetID) const;

	UFUNCTION(BlueprintPure, Category = "Jupiter|Presets")
	bool IsPresetNameTaken(FName PresetName) const;

	UFUNCTION(Server, Reliable, Category = "Jupiter|Presets")
	void Server_DeletePreset(FGuid PresetID);

	UFUNCTION(Server, Reliable, Category = "Jupiter|Presets")
	void Server_RenamePreset(FGuid PresetID, FName NewName);

	UFUNCTION(Server, Reliable, Category = "Jupiter|Presets")
	void Server_SpawnPreset(FGuid PresetID, FVector Location, FRotator Rotation);

	UFUNCTION(Server, Reliable, Category = "Jupiter|Presets")
	void Server_ReplicatePreset(const FPlacementPreset& NewPreset);

	
	UFUNCTION(BlueprintCallable, Category = "Jupiter|Presets")
	void SaveToDisk();

	UFUNCTION(BlueprintCallable, Category = "Jupiter|Presets")
	void LoadFromDisk();

    void OnAsyncSaveFinished(const FString& SlotName, const int32 UserIndex, bool bSuccess);
	
	
	UPROPERTY(BlueprintAssignable, Category = "Settings|Presets")
	FOnPresetsChanged OnPresetsChanged;

private:
    bool ValidateSpawnRequest(APlayerController* PC, FGuid PresetID, FVector Location);

	UPROPERTY(ReplicatedUsing=OnRep_SavedPresets)
	TArray<FPlacementPreset> SavedPresets;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Presets")
	FString SaveSlotName = "JupiterPresets";

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Presets")
	bool bResetSavesOnBeginPlay = false;
    
    TMap<TWeakObjectPtr<APlayerController>, double> SpawnCooldowns;

    UPROPERTY(EditDefaultsOnly, Category = "Settings|Validation")
    float MinSpawnCooldown = 1.0f;
};
