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

USTRUCT(BlueprintType)
struct FPatrolRoute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> PatrolPoints;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsLoop = false;
};


UCLASS(ClassGroup=(RTS), meta=(BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UUnitPatrolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUnitPatrolComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	const TArray<FPatrolRoute>& GetActiveRoutes() const { return ActivePatrolRoutes; }

protected:
	// ============================================================
	// INTERNAL HELPERS
	// ============================================================

	void EnsureVisualizerComponent();

	void UpdateVisualization();

	static FPatrolRouteExtended ConvertToExtended(const FPatrolRoute& Route, const FLinearColor& Color = FLinearColor::Blue);

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
