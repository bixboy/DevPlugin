// Copyright 2026

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FirstPersonBuilderTestCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UPlayerConstructionObject;
class UPlacementPropData;
class UConstructionSystemSettings;

UCLASS()
class FIRSTPERSONBUILDER_API AFirstPersonBuilderTestCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFirstPersonBuilderTestCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// --- Components ---

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	TObjectPtr<USkeletalMeshComponent> Mesh1P;

	/** The global settings for the construction system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
	TObjectPtr<UConstructionSystemSettings> ConstructionSettings;

	/** Visual theme and icons for the catalog menu */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|UI")
	TObjectPtr<class UBuilderMenuTheme> MenuTheme;

	/** The core construction object, replicated only to owner */
	UPROPERTY(ReplicatedUsing = OnRep_ConstructionObject, Transient, BlueprintReadOnly, Category = "Construction")
	TObjectPtr<UPlayerConstructionObject> ConstructionObject;

	UFUNCTION()
	void OnRep_ConstructionObject();

public:
	UFUNCTION(BlueprintCallable, Category = "Construction")
	void LeftClick_Pressed();

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void OpenBuildMenu(const TArray<UPlacementPropData*>& Recipes, class UBuilderMenuTheme* InMenuStyle = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void CloseBuildMenu();

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void RotateGhost(int32 Direction);
};
