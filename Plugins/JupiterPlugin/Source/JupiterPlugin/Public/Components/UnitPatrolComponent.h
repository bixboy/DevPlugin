#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "Data/AiData.h"
#include "Data/PatrolData.h"
#include "UnitPatrolComponent.generated.h"

class UPatrolVisualizerComponent;
class UUnitOrderComponent;
class UUnitSelectionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPatrolRoutesChanged);


USTRUCT(BlueprintType)
struct FPatrolRoute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid PatrolID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> PatrolPoints;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPatrolType PatrolType = EPatrolType::Loop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RouteName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor RouteColor = FLinearColor::Blue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WaitTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShowArrows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShowNumbers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<AActor>> AssignedUnits;
};


USTRUCT(BlueprintType)
struct FPatrolCreationParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FVector> Points;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<AActor*> Units;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EPatrolType Type = EPatrolType::Loop;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName Name = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLinearColor Color = FLinearColor::Blue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowArrows = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowNumbers = true;
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
	const TArray<FPatrolRoute>& GetActiveRoutes() const { return ActivePatrolRoutes; }
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_CreatePatrol(const FPatrolCreationParams& Params);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_UpdatePatrolRoute(int32 Index, const FPatrolRoute& NewRoute);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_UpdateUnitPatrol(AActor* Unit, const FPatrolRoute& NewRoute);

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

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_ReversePatrolRouteByID(FGuid PatrolID);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
	void Server_RenamePatrol(FGuid PatrolID, FName NewName);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Patrol")
    void Server_SetPatrolType(FGuid PatrolID, EPatrolType NewType);

	/** Multicast to ensure all clients clear their cache/state for ignored patrol */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_RemovePatrolRoute(const TArray<AActor*>& Units, const FGuid& PatrolID);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPatrolRoutesChanged OnPatrolRoutesChanged;

protected:

	bool GetPatrolRouteForUnit(AActor* Unit, FPatrolRoute& OutRoute) const;

	void StopUnits(const TArray<AActor*>& Units);

	void StopUnit(AActor* Unit);

	// ============================================================
	// INTERNAL HELPERS
	// ============================================================

	void EnsureVisualizerComponent();

	void UpdateVisualization();

	static FPatrolRouteExtended ConvertToExtended(const FPatrolRoute& Route, const FLinearColor& Color = FLinearColor::Blue);

	/** Helper to resolve logical path index for seamless updates */
	int32 GetTargetPointIndexForUnit(AActor* Unit, int32 NewPathSize, bool bIsReverse) const;

	/** Helper to centralize command issuance logic */
	void UpdatePatrolOrderForUnit(AActor* Unit, const FPatrolRoute& Route, bool bIsReverseUpdate);

	bool IsLocallyControlled() const;

	bool HasAuthority() const;

	// ============================================================
	// EVENT HANDLERS
	// ============================================================

	UFUNCTION()
	void HandleOrdersDispatched(const TArray<AActor*>& AffectedUnits, const FCommandData& IssuedCommand);

	UFUNCTION()
	void OnSelectionChanged(const TArray<AActor*>& SelectedActors);

	void RefreshRoutesFromSelection();

	void ApplyRoutes(const TArray<FPatrolRoute>& NewRoutes);

	// ============================================================
	// DATA - REPLICATED
	// ============================================================

	UPROPERTY(ReplicatedUsing=OnRep_ActivePatrolRoutes)
	TArray<FPatrolRoute> ActivePatrolRoutes;

	UFUNCTION()
	void OnRep_ActivePatrolRoutes();

	UPROPERTY()
	TMap<AActor*, FPatrolRoute> UnitRouteCache;

	// ============================================================
	// DATA - LOCAL
	// ============================================================

	UPROPERTY(Transient)
	TObjectPtr<UPatrolVisualizerComponent> PatrolVisualizer = nullptr;

	UPROPERTY()
	TObjectPtr<UUnitOrderComponent> OrderComponent = nullptr;

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
