#pragma once

#include "CoreMinimal.h"
#include "Data/AiData.h"
#include "GameFramework/Character.h"
#include "Interfaces/Damageable.h"
#include "Interfaces/Selectable.h"

#include "SoldierRts.generated.h"

class USoldierManagerComponent;
class UWeaponMaster;
class UCommandComponent;
class AAiControllerRts;

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActionEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBehaviorUpdatedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSelectedDelegate, bool, bIsSelected);

UCLASS(Blueprintable)
class JUPITERPLUGIN_API ASoldierRts : public ACharacter, public ISelectable, public IDamageable
{
    GENERATED_BODY()

public:
    ASoldierRts(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // AActor interface
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PossessedBy(AController* NewController) override;

    // ISelectable interface
    virtual void Select() override;
    virtual void Deselect() override;
    virtual void Highlight(bool bHighlight) override;
    virtual bool GetIsSelected_Implementation() override;
    virtual void CommandMove_Implementation(FCommandData CommandData) override;
    virtual FCommandData GetCurrentCommand_Implementation() override;
    virtual void SetBehavior_Implementation(ECombatBehavior NewBehavior) override;
    virtual ECombatBehavior GetBehavior_Implementation() override;
    virtual ETeams GetCurrentTeam_Implementation() override;

    // IDamageable interface
    virtual void TakeDamage_Implementation(AActor* DamageOwner) override;
    virtual bool GetIsInAttack_Implementation() override;
    virtual bool GetCanAttack_Implementation() override;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    UCommandComponent* GetCommandComponent() const;

    UFUNCTION(BlueprintCallable)
    void SetAIController(AAiControllerRts* AiController);

    UFUNCTION(BlueprintCallable, BlueprintPure)
    AAiControllerRts* GetAiController() const;

    // Networking helpers
    UFUNCTION(NetMulticast, Unreliable)
    void NetMulticast_Unreliable_CallOnStartAttack();

    UFUNCTION(NetMulticast, Unreliable)
    void NetMulticast_Unreliable_CallOnStartWalking();

    UFUNCTION(NetMulticast, Unreliable)
    void NetMulticast_Unreliable_CallOnEndWalking();

    // Attack accessors
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetAttackRange() const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetAllyDetectionRange() const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetAttackCooldown() const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetRangedStopDistance() const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetMeleeStopDistanceFactor() const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    ECombatBehavior GetCombatBehavior() const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsFriendlyActor(AActor* Actor) const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsEnemyActor(AActor* Actor) const;

    UFUNCTION()
    void ProcessDetectionResults(TArray<AActor*> NewEnemies, TArray<AActor*> NewAllies);

    // Weapon helpers
    UFUNCTION(BlueprintCallable, BlueprintPure)
    UWeaponMaster* GetCurrentWeapon();

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool GetHaveWeapon();

    // Delegates
    UPROPERTY(BlueprintAssignable)
    FActionEvent AttackEvent_Delegate;

    UPROPERTY(BlueprintAssignable)
    FActionEvent StartWalkingEvent_Delegate;

    UPROPERTY(BlueprintAssignable)
    FActionEvent EndWalkingEvent_Delegate;

    UPROPERTY(BlueprintAssignable)
    FSelectedDelegate OnSelected;

    UPROPERTY()
    FBehaviorUpdatedDelegate OnBehaviorUpdate;

protected:
    void TryRegisterPrompt();

    UFUNCTION()
    void OnStartAttack(AActor* Target);

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

    // Components and references
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCommandComponent> CommandComp;

    UPROPERTY()
    TObjectPtr<AAiControllerRts> AIController = nullptr;

    UPROPERTY()
    TObjectPtr<USoldierManagerComponent> SoldierManager = nullptr;

    // Selection state
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0", ClampMax = "255", AllowPrivateAccess = "true"))
    int32 SelectionStencilValue = 1;

    UPROPERTY()
    bool bIsSelected = false;

    // Movement state
    UPROPERTY()
    bool bIsMoving = false;

    // AI configuration
    UPROPERTY(EditAnywhere, Category = "Settings|DefaultValue")
    TSubclassOf<AAiControllerRts> AiControllerRtsClass;

    // Attack configuration
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
    
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Attack", ReplicatedUsing = OnRep_CombatBehavior)
    ECombatBehavior CombatBehavior = ECombatBehavior::Passive;

private:
    UPROPERTY()
    TArray<AActor*> ActorsInRange;

    UPROPERTY()
    TArray<AActor*> AllyInRange;

    // Team configuration
    UPROPERTY(EditAnywhere, Category = "Settings|Team")
    ETeams CurrentTeam = ETeams::Clone;

    // Weapons
    UPROPERTY(EditAnywhere, Category = "Settings|Weapons")
    TSubclassOf<UWeaponMaster> WeaponClass;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
    UWeaponMaster* CurrentWeapon = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Settings|Weapons", meta = (AllowPrivateAccess = "true"))
    bool bHasWeapon = false;
};
