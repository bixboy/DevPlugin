#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/PatrolData.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "PatrolVisualizerComponent.generated.h"

class UCameraComponent;
class UTextRenderComponent;
class UPatrolSystemSettings;
struct FPatrolRouteExtended;

struct FPatrolInstanceTracker
{
    TObjectPtr<UInstancedStaticMeshComponent> ISMComponent;

    TMap<FGuid, TArray<int32>> RouteToIndices;
    TMap<int32, FGuid> IndexToRoute;

    void SetComponent(UInstancedStaticMeshComponent* InComp);
    void Clear();
    void AddInstances(const FGuid& RouteID, const TArray<FTransform>& Transforms, const TArray<float>& CustomData1, const TArray<float>& CustomData2, const TArray<float>& CustomData3);
    void RemoveInstances(const FGuid& RouteID);
    bool FindRouteAndIndex(int32 InstanceIndex, FGuid& OutRouteID, int32& OutPointIndex) const;
};

USTRUCT()
struct FPatrolVisualizationCache
{
    GENERATED_BODY()

    TArray<FVector> ElevatedPoints;
    TArray<FVector> SplineSamples;
    FBox RouteBounds = FBox(ForceInit);
    
    uint32 GeometryHash = 0;
    EPatrolType PatrolTypeCached = EPatrolType::Once;
    EPatrolVisualizationState State = EPatrolVisualizationState::Active;
    float LastAccessTime = 0.f;

    void Reset()
    {
        ElevatedPoints.Reset();
        SplineSamples.Reset();
        RouteBounds.Init();
        GeometryHash = 0;
        PatrolTypeCached = EPatrolType::Once;
        State = EPatrolVisualizationState::Active;
        LastAccessTime = 0.f;
    }
};


UCLASS(ClassGroup=(RTS), meta=(BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UPatrolVisualizerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPatrolVisualizerComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // --- Public API ---
    UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
    void UpdateVisualization(const TArray<FPatrolRouteExtended>& Routes);

    UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
    bool GetPatrolPointFromHitIndex(int32 HitIndex, FGuid& OutPatrolID, int32& OutPointIndex) const;

    UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
    void UpdatePointPosition(FGuid PatrolID, int32 PointIndex, FVector NewLocation); // For Local visual feedback

    UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
    void SetVisibility(bool bVisible);

protected:
	
    // --- Core ---
    void EnsureComponents(const UPatrolSystemSettings* Settings);

    void RebuildGeometry();

    void RenderRouteCached(const FPatrolRouteExtended& Route, const UPatrolSystemSettings* Settings);

    // --- Drawing Helpers ---
    void DrawSplinePolyline(const FGuid& RouteID, const TArray<FVector>& Samples, const FLinearColor& Color, float Thickness, bool bDashed = false);

    void DrawWaypoints(const FGuid& RouteID, const TArray<FVector>& Points, const FLinearColor& Color, const FPatrolRouteExtended& Route, const UPatrolSystemSettings* Settings);
   
    void DrawDirectionArrows(const FGuid& RouteID, const TArray<FVector>& Samples, const FLinearColor& Color, const UPatrolSystemSettings* Settings);

    // --- Geometry ---
    void BuildSplineSamples(const TArray<FVector>& Points, EPatrolType Type, TArray<FVector>& OutSamples, const UPatrolSystemSettings* Settings);
    
    static FVector EvaluateCatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T);
    
    static uint32 HashGeometry(const TArray<FVector>& Points, EPatrolType Type);
    
    static FBox CalculateRouteBounds(const TArray<FVector>& Points);

    // --- LOD & Utilities ---
    void UpdateLOD(const UPatrolSystemSettings* Settings);
    
    bool ShouldCullRoute(const FBox& RouteBounds, const UPatrolSystemSettings* Settings) const;
    
    float GetDistanceToCamera() const;
    
    UCameraComponent* GetViewCamera() const;

    // --- Visual Helpers --- 
    FLinearColor GetRouteColor(const FPatrolRouteExtended& Route, EPatrolVisualizationState State, const UPatrolSystemSettings* Settings) const;
    
    FLinearColor ApplyColorAnimation(const FLinearColor& BaseColor, float Time, const UPatrolSystemSettings* Settings) const;
    
    bool ShouldAnimate(const UPatrolSystemSettings* Settings) const;

protected:
    // --- Data ---
    UPROPERTY()
    TArray<FPatrolRouteExtended> CurrentRoutes;

    // Cache keyed by PatrolID
    TMap<FGuid, FPatrolVisualizationCache> VisualizationCache;
    
    // Trackers
    FPatrolInstanceTracker WaypointTracker;
    FPatrolInstanceTracker ArrowTracker;
    FPatrolInstanceTracker PathLineTracker;

    // --- Components ---
    UPROPERTY(Transient)
    TObjectPtr<UInstancedStaticMeshComponent> PathLineISM = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UInstancedStaticMeshComponent> WaypointISM = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UInstancedStaticMeshComponent> ArrowISM = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextRenderComponent>> TextLabelPool;

    // --- State ---
    int32 ActiveTextLabels = 0;
    float GlobalAnimationTime = 0.f;
    float RefreshTimer = 0.f;
    bool bVisualsDirty = true;
    int32 CurrentLODLevel = 0;
    float LastCameraDistance = 0.f;

	UPROPERTY(Transient)
	EPatrolVisualQuality CurrentQuality = EPatrolVisualQuality::High;
};