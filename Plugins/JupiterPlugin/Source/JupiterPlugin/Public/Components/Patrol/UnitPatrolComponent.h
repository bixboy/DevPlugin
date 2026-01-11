#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "Data/PatrolData.h"
#include "UnitPatrolComponent.generated.h"

class UPatrolVisualizerComponent;
class UUnitSelectionComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPatrolRoutesChanged);

UENUM(BlueprintType)
enum class EPatrolModAction : uint8
{
    Rename,
    ChangeType,
    ChangeColor,
    Reverse
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
	void Server_RemovePatrolRoute(int32 Index);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_RemovePatrolRouteByID(FGuid PatrolID);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_RemovePatrolRouteForUnit(AActor* Unit);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_ReversePatrolRoute(int32 Index);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_ReversePatrolRouteForUnit(AActor* Unit);

    /** Master RPC for all patrol modifications to reduce function bloat */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
    void Server_ModifyPatrol(FGuid PatrolID, EPatrolModAction Action, const FPatrolModPayload& Payload);

	/** Multicast to ensure all clients clear their cache/state for ignored patrol */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_RemovePatrolRoute(const TArray<AActor*>& Units, const FGuid& PatrolID);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPatrolRoutesChanged OnPatrolRoutesChanged;

	// ============================================================
	// CALLBACKS (Public for Struct access)
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

	/** Helper to resolve logical path index for seamless updates */
	int32 GetTargetPointIndexForUnit(AActor* Unit, int32 NewPathSize, bool bIsReverse) const;

	bool IsLocallyControlled() const;

	bool HasAuthority() const;

	// ============================================================
	// EVENT HANDLERS
	// ============================================================

	UFUNCTION()
	void OnSelectionChanged(const TArray<AActor*>& SelectedActors);

	void RefreshRoutesFromSelection();

    /** Helper to push updates to AI */
    void NotifyPatrolUpdate(const FPatrolRoute& Route, bool bIsReverse);

	void ApplyRoutes(const TArray<FPatrolRoute>& NewRoutes);
	void OnRep_ActivePatrolRoutes();

	// ============================================================
	// DATA - REPLICATED
	// ============================================================

	UPROPERTY(Replicated)
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

	// ============================================================
	// SETTINGS (exposed for per-instance customization)
	// ============================================================

	UPROPERTY(EditAnywhere, Category = "Settings|Patrol")
	FLinearColor RouteColor = FLinearColor(0.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Settings|Patrol")
	bool bAutoCreateVisualizer = true;
};
