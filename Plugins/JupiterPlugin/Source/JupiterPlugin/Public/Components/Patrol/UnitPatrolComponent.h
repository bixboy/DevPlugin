#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "Data/PatrolData.h"
#include "UnitPatrolComponent.generated.h"

class UPatrolVisualizerComponent;
class UUnitSelectionComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPatrolRoutesChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatrolSelected, const FGuid&, PatrolID);

UENUM(BlueprintType)
enum class EPatrolModAction : uint8
{
    Rename,
    ChangeType,
    ChangeColor,
    Reverse
};

UENUM(BlueprintType)
enum class EPatrolDeleteOption : uint8
{
    Disband,
    JoinNearest
};

USTRUCT(BlueprintType)
struct FPatrolModPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName NewName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EPatrolType NewType = EPatrolType::Loop;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FLinearColor NewColor = FLinearColor::White;
};

UCLASS(ClassGroup=(RTS), meta=(BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UUnitPatrolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUnitPatrolComponent();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	const TArray<FPatrolRouteItem>& GetActiveRoutes() const { return PatrolRoutes.Items; }
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_CreatePatrol(const FPatrolCreationParams& Params);

	bool GetPatrolRouteForUnit(AActor* Unit, FPatrolRoute& OutRoute) const;
	FGuid CreatePatrol(const FPatrolCreationParams& Params);


	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_UpdatePatrolRoute(int32 Index, const FPatrolRoute& NewRoute);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
    void Server_UpdatePatrolPoint(FGuid PatrolID, int32 PointIndex, FVector NewLocation);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_RemovePatrolRoute(int32 Index);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_RemovePatrolRouteByID(FGuid PatrolID);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
    void Server_RemovePatrolWithOption(FGuid PatrolID, EPatrolDeleteOption Option);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_RemovePatrolRouteForUnit(AActor* Unit);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_ReversePatrolRoute(int32 Index);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_ReversePatrolRouteForUnit(AActor* Unit);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
    void Server_ModifyPatrol(FGuid PatrolID, EPatrolModAction Action, const FPatrolModPayload& Payload);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
    void Server_AssignUnitsToPatrol(const TArray<AActor*>& Units, FGuid PatrolID);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_RemovePatrolRoute(const TArray<AActor*>& Units, const FGuid& PatrolID);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPatrolRoutesChanged OnPatrolRoutesChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPatrolSelected OnPatrolSelected;

	// ============================================================
	// CALLBACKS
	// ============================================================

	UFUNCTION()
	void OnPatrolRouteAdded(const FPatrolRoute& Route);

	UFUNCTION()
	void OnPatrolRouteChanged(const FPatrolRoute& Route);

	UFUNCTION()
	void OnPatrolRouteRemoved(const FPatrolRoute& Route);

protected:

	// ============================================================
	// INTERNAL HELPERS
	// ============================================================

	void EnsureVisualizerComponent();

	void UpdateVisualization();

	static FPatrolRouteExtended ConvertToExtended(const FPatrolRoute& Route, const FLinearColor& Color = FLinearColor::Blue);

	int32 GetTargetPointIndexForUnit(AActor* Unit, int32 NewPathSize, bool bIsReverse) const;

	bool IsLocallyControlled() const;

	bool HasAuthority() const;

	// ============================================================
	// EVENT HANDLERS
	// ============================================================

	UFUNCTION()
	void OnSelectionChanged(const TArray<AActor*>& SelectedActors);

    UFUNCTION()
    void OnUnitDestroyed(AActor* DestroyedActor);

	void RefreshRoutesFromSelection();

    void NotifyPatrolUpdate(const FPatrolRoute& Route, bool bIsReverse);

	void ApplyRoutes(const TArray<FPatrolRoute>& NewRoutes);
	
	UFUNCTION()
	void OnRep_ActivePatrolRoutes();

	// ============================================================
	// DATA - REPLICATED
	// ============================================================

	UPROPERTY(ReplicatedUsing=OnRep_ActivePatrolRoutes)
	FPatrolRouteArray PatrolRoutes;

	UPROPERTY()
	TMap<TWeakObjectPtr<AActor>, FGuid> UnitToRouteMap;

    void RebuildUnitLookupMap();

	// ============================================================
	// DATA - LOCAL
	// ============================================================

	UPROPERTY(Transient)
	TObjectPtr<UPatrolVisualizerComponent> PatrolVisualizer = nullptr;

	UPROPERTY()
	TObjectPtr<UUnitSelectionComponent> SelectionComponent = nullptr;

    UPROPERTY(Transient)
    FGuid UISelectedPatrolID;

public:
    UFUNCTION(BlueprintCallable, Category = "Patrol")
    void SetUISelectedPatrol(FGuid PatrolID);

	// ============================================================
	// SETTINGS
	// ============================================================

	UPROPERTY(EditAnywhere, Category = "Settings|Patrol")
	FLinearColor RouteColor = FLinearColor(0.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Settings|Patrol")
	bool bAutoCreateVisualizer = true;
};
