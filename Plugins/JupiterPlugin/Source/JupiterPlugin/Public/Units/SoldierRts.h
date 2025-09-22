#pragma once
#include "CoreMinimal.h"
#include "Data/AiData.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/Damageable.h"
#include "Interfaces/Selectable.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

#include "SoldierRts.generated.h"

class USoldierManagerComponent;

USTRUCT(BlueprintType)
struct FAttackDetectionSettings
{
    GENERATED_BODY()

    /** How often the detection should be refreshed when using the calculation based detection. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (AllowPrivateAccess = "true", ClampMin = "0.05"))
    float RefreshInterval = 0.25f;

    /** Maximum number of enemies to keep in range. 0 means no limit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
    int32 MaxEnemiesTracked = 0;

    /** Maximum number of allies to keep in range. 0 means no limit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
    int32 MaxAlliesTracked = 0;

    /** When true the tracked actors are sorted by distance before being stored. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (AllowPrivateAccess = "true"))
    bool bPrioritizeClosestTargets = true;

    /** Enable to visualize the attack range and detected actors. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Debug", meta = (AllowPrivateAccess = "true"))
    bool bDebugDrawDetection = false;

    /** Color used when drawing the debug information. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Debug", meta = (AllowPrivateAccess = "true", EditCondition = "bDebugDrawDetection"))
    FLinearColor DebugColor = FLinearColor::Red;

    /** Duration in seconds for which debug shapes stay visible. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Debug", meta = (AllowPrivateAccess = "true", EditCondition = "bDebugDrawDetection", ClampMin = "0.0"))
    float DebugDuration = 0.1f;

    /** Draw a line to every detected target when debugging. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Debug", meta = (AllowPrivateAccess = "true", EditCondition = "bDebugDrawDetection"))
    bool bDrawTargetLines = true;

    /** Thickness of the debug lines when enabled. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Debug", meta = (AllowPrivateAccess = "true", EditCondition = "bDebugDrawDetection", ClampMin = "0.0"))
    float DebugLineThickness = 1.5f;
};

class UWeaponMaster;
class USphereComponent;
class UCommandComponent;
class AAiControllerRts;
class UCharacterMovementComponent;
class APlayerControllerRts;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActionEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBehaviorUpdatedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSelectedDelegate, bool, bIsSelected);

UCLASS(Blueprintable)
class JUPITERPLUGIN_API ASoldierRts : public ACharacter, public ISelectable, public IDamageable
{
	GENERATED_BODY()

public:
	ASoldierRts(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;
	
	virtual void PossessedBy(AController* NewController) override;

	virtual FCommandData GetCurrentCommand_Implementation() override;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	UCommandComponent* GetCommandComponent() const;

	
protected:
    UFUNCTION()
    virtual void OnConstruction(const FTransform& Transform) override;
    UFUNCTION()
    virtual void Destroyed() override;
	UFUNCTION()
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void TryRegisterPrompt();

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = true))
	TObjectPtr<UCommandComponent> CommandComp;

	UPROPERTY()
	bool bIsMoving;

	UPROPERTY()
	USoldierManagerComponent* SoldierManager;
	
public:
	UPROPERTY(BlueprintAssignable)
	FActionEvent AttackEvent_Delegate;

	UPROPERTY(BlueprintAssignable)
	FActionEvent StartWalkingEvent_Delegate;
	
	UPROPERTY(BlueprintAssignable)
	FActionEvent EndWalkingEvent_Delegate;


#pragma region Net Functions

	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_Unreliable_CallOnStartAttack();

	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_Unreliable_CallOnStartWalking();

	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_Unreliable_CallOnEndWalking();

#pragma endregion	
	
// AI controller
#pragma region AI Controller
	
public:
	UFUNCTION()
	void SetAIController(AAiControllerRts* AiController);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	AAiControllerRts* GetAiController() const;
	
protected:
	UPROPERTY()
	TObjectPtr<AAiControllerRts> AIController;
	
	UPROPERTY(EditAnywhere, Category = "Settings|DefaultValue")
	TSubclassOf<AAiControllerRts> AiControllerRtsClass;

#pragma endregion	

// Selection	
#pragma region Selection
	
public:
	/*- Function -*/
	virtual void Select() override;
	virtual void Deselect() override;
	virtual void Highlight(const bool Highlight) override;
	
	UFUNCTION()
	virtual bool GetIsSelected_Implementation() override;

	UPROPERTY()
	FSelectedDelegate OnSelected;
	
protected:
        /*- Variables -*/
        UPROPERTY()
        bool Selected;

        UPROPERTY()
        TObjectPtr<APlayerControllerRts> PlayerOwner;

        /** Custom depth stencil value applied when the unit is highlighted. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0", ClampMax = "255"))
        int32 SelectionStencilValue = 1;

#pragma endregion

// Movement	
#pragma region Movement
public:
	UFUNCTION()
	virtual void CommandMove_Implementation(FCommandData CommandData) override;

#pragma endregion	

// Attack
#pragma region Attack

public:
	
	/*- Interface -*/
	virtual void TakeDamage_Implementation(AActor* DamageOwner) override;
	
	virtual bool GetIsInAttack_Implementation() override;

	virtual bool GetCanAttack_Implementation() override;
	
	/*- Getter -*/
	UFUNCTION()
	float GetAttackRange() const;

	UFUNCTION()
	float GetAllyDetectionRange() const;

	UFUNCTION()
	float GetAttackCooldown() const;

	UFUNCTION()
	bool IsFriendlyActor(AActor* Actor) const;

	UFUNCTION()
	bool IsEnemyActor(AActor* Actor) const;

	UFUNCTION()
	ECombatBehavior GetCombatBehavior() const;

	UFUNCTION()
	float GetRangedStopDistance() const;

	UFUNCTION()
	float GetMeleeStopDistanceFactor() const;

	UFUNCTION()
	void ProcessDetectionResults(TArray<AActor*> NewEnemies, TArray<AActor*> NewAllies);
	
protected:

    /*- Function -*/
	UFUNCTION()
	virtual void SetBehavior_Implementation(const ECombatBehavior NewBehavior) override;

	UFUNCTION()
	virtual ECombatBehavior GetBehavior_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Attack", ReplicatedUsing = OnRep_CombatBehavior)
	ECombatBehavior CombatBehavior = ECombatBehavior::Passive;

	UFUNCTION()
	virtual void OnStartAttack(AActor* Target);

        UFUNCTION()
        void UpdateActorsInArea();

        UFUNCTION()
        void OnRep_CombatBehavior();

        void DrawAttackDebug(const TArray<AActor*>& DetectedEnemies, const TArray<AActor*>& DetectedAllies) const;

private:
    bool IsValidSelectableActor(const AActor* Actor) const;
    bool ShouldAutoEngage() const;
    FCommandData MakeAttackCommand(AActor* Target) const;
    void IssueAttackOrder(const FCommandData& CommandData);
    void HandleAutoEngage(AActor* Target);
    void HandleTargetRemoval(AActor* OtherActor);
    void NotifyAlliesOfThreat(AActor* Threat, const FCommandData& CommandData);


    /*- Variables -*/
	UPROPERTY(EditAnywhere, Category = "Settings|Attack")
	bool bCanAttack = true;

	UPROPERTY(EditAnywhere, Category = "Settings|Attack")
	float AttackCooldown = 1.5f;

    UPROPERTY(EditAnywhere, Category = "Settings|Attack")
    float AttackRange = 200.f;

	UPROPERTY(EditAnywhere, Category = "Settings|Attack", meta = (ClampMin = "0.0"))
	float RangedStopDistance = 200.f;

	UPROPERTY(EditAnywhere, Category = "Settings|Attack", meta = (ClampMin = "0.0"))
	float MeleeStopDistanceFactor = 1.f;

    UPROPERTY(EditAnywhere, Category = "Settings|Attack")
    float AllyDetectionRange = 200.f;

    UPROPERTY(EditAnywhere, Category = "Settings|Attack")
    FAttackDetectionSettings DetectionSettings;

    UPROPERTY()
    float DetectionElapsedTime = 0.f;

    UPROPERTY()
    TArray<AActor*> ActorsInRange;

    UPROPERTY()
    TArray<AActor*> AllyInRange;

	UPROPERTY()
	FBehaviorUpdatedDelegate OnBehaviorUpdate;

#pragma endregion

// Team	
#pragma region Team
public:
	UFUNCTION(BlueprintCallable)
	virtual ETeams GetCurrentTeam_Implementation() override;

protected:
	UPROPERTY(EditAnywhere, Category = "Settings|Team")
	ETeams CurrentTeam = ETeams::Clone;

#pragma endregion

// Weapons
#pragma region Weapons
	
public:
	UFUNCTION()
	UWeaponMaster* GetCurrentWeapon();
	
	UFUNCTION()
	bool GetHaveWeapon();
	
protected:
	UPROPERTY(EditAnywhere, Category = "Settings|Weapons")
	TSubclassOf<UWeaponMaster> WeaponClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = true))
	UWeaponMaster* CurrentWeapon;
	
	UPROPERTY(BlueprintReadWrite)
	bool HaveWeapon;

#pragma endregion
};
