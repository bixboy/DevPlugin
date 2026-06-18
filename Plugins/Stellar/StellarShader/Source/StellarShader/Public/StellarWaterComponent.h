// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StellarWaterComponent.generated.h"

class UMaterialInstanceDynamic;
class UMeshComponent;
class UMaterialInterface;

/**
 * UStellarWaterComponent
 *
 * Parameterizable water shader controller.
 * Attach to any actor with a mesh, set BaseMaterial, and configure water parameters.
 * Creates a UMaterialInstanceDynamic and syncs all parameters automatically.
 *
 * Use AStellarWaterActor for a drag-and-drop solution with a built-in plane.
 */
UCLASS(ClassGroup = (StellarShader), meta = (BlueprintSpawnableComponent, DisplayName = "Stellar Water"))
class STELLARSHADER_API UStellarWaterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStellarWaterComponent();

	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// ---- Base Material ----

	/** Base water material to create MID from. If null, uses the material builder output. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Material")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** Target mesh component to apply the water material to. Auto-detected if null. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Material")
	TObjectPtr<UMeshComponent> TargetMesh;

	// ---- Wave Parameters ----

	/** Intensity of the physical 3D mesh deformation (0 = 2D flat water, 1 = full 3D waves) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Waves", meta = (ClampMin = "0", ClampMax = "1"))
	float DeformationIntensity = 1.0f;

	/** Height of the waves in world units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Waves", meta = (ClampMin = "0", UIMin = "0", UIMax = "500"))
	float WaveAmplitude = 100.0f;

	/** Distance between wave crests in cm (higher = wider waves) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Waves", meta = (ClampMin = "10.0", UIMin = "100.0", UIMax = "10000.0"))
	float WaveLength = 1500.0f;

	/** Animation speed multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Waves", meta = (ClampMin = "0", UIMin = "0", UIMax = "5.0"))
	float WaveSpeed = 1.0f;

	/** 0 = smooth sine waves, 1 = sharp Gerstner peaks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Waves", meta = (ClampMin = "0", ClampMax = "1"))
	float WaveSteepness = 0.5f;

	/** Primary wave travel direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Waves")
	FVector2D WaveDirection = FVector2D(1.0, 0.0);

	/** 0 = waves from all directions, 1 = waves aligned to WaveDirection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Waves", meta = (ClampMin = "0", ClampMax = "1"))
	float WaveDirectionality = 0.3f;

	/** Number of wave layers (more = richer, heavier on GPU) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Waves", meta = (ClampMin = "1", ClampMax = "12", UIMin = "1", UIMax = "12"))
	int32 NumWaves = 5;

	/** Scale of high-frequency detail waves */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Waves", meta = (ClampMin = "0", UIMin = "0", UIMax = "1.0"))
	float DetailWaveScale = 0.3f;

	// ---- Color Parameters ----

	/** Water color in shallow areas */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Color")
	FLinearColor ShallowColor = FLinearColor(0.1f, 0.6f, 0.7f, 1.0f);

	/** Water color in deep areas */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Color")
	FLinearColor DeepColor = FLinearColor(0.02f, 0.1f, 0.2f, 1.0f);

	/** Speed of shallow-to-deep color transition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Color", meta = (ClampMin = "0.1", UIMin = "0.1", UIMax = "10.0"))
	float DepthFalloff = 2.0f;

	/** Base water opacity (before Fresnel) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Color", meta = (ClampMin = "0", ClampMax = "1"))
	float WaterOpacity = 0.85f;

	/** Fresnel power — higher values = more transparent when viewed head-on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Color", meta = (ClampMin = "0.5", UIMin = "0.5", UIMax = "10.0"))
	float FresnelPower = 3.0f;

	// ---- Surface Parameters ----

	/** Surface roughness (0.02 = mirror-like, 0.5 = matte) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Surface", meta = (ClampMin = "0", ClampMax = "1"))
	float Roughness = 0.02f;

	/** Intensity of wave normal perturbation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Surface", meta = (ClampMin = "0", UIMin = "0", UIMax = "3.0"))
	float NormalStrength = 1.0f;

	// ---- Foam Parameters ----

	/** Color of the foam */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Foam")
	FLinearColor FoamColor = FLinearColor::White;

	/** Amount of foam (0 = none, 1 = maximum) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Foam", meta = (ClampMin = "0", ClampMax = "1"))
	float FoamAmount = 0.3f;

	/** Scale of foam noise pattern */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Foam", meta = (ClampMin = "0.1", UIMin = "0.1", UIMax = "10.0"))
	float FoamScale = 2.0f;

	/** Foam animation speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Foam", meta = (ClampMin = "0", UIMin = "0", UIMax = "3.0"))
	float FoamSpeed = 0.5f;

	// ---- Caustics Parameters ----

	/** Caustics brightness (0 = off) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Caustics", meta = (ClampMin = "0", ClampMax = "1"))
	float CausticsIntensity = 0.3f;

	/** Scale of the caustic pattern */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Caustics", meta = (ClampMin = "0.1", UIMin = "0.1", UIMax = "10.0"))
	float CausticsScale = 3.0f;

	/** Caustics animation speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Caustics", meta = (ClampMin = "0", UIMin = "0", UIMax = "5.0"))
	float CausticsSpeed = 0.8f;

	// ---- Blueprint API ----

	/** Apply all current parameters to the material instance */
	UFUNCTION(BlueprintCallable, Category = "Water")
	void ApplyParameters();

	/** Get the active material instance */
	UFUNCTION(BlueprintPure, Category = "Water")
	UMaterialInstanceDynamic* GetWaterMID() const { return WaterMID; }

public:
	void InitializeMaterial();
	void SyncAllParameters();

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> WaterMID;
};
