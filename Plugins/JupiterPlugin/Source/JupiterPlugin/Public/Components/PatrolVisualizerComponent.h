#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/PatrolData.h"
#include "PatrolVisualizerComponent.generated.h"

class ULineBatchComponent;
class UCameraComponent;
class UInstancedStaticMeshComponent;
class UTextRenderComponent;
class UPatrolSystemSettings;
struct FPatrolRouteExtended;


USTRUCT()
struct FPatrolVisualizationCache
{
    GENERATED_BODY()

    TArray<FVector> ElevatedPoints;
    TArray<FVector> SplineSamples;
    FBox RouteBounds = FBox(ForceInit);
    
    uint32 GeometryHash = 0;
    bool bLoopCached = false;
    EPatrolVisualizationState State = EPatrolVisualizationState::Active;
    float LastAccessTime = 0.f;

    void Reset()
    {
        ElevatedPoints.Reset();
        SplineSamples.Reset();
        RouteBounds.Init();
        GeometryHash = 0;
        bLoopCached = false;
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
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // --- Public API ---
    UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
    void UpdateVisualization(const TArray<FPatrolRouteExtended>& Routes);

    UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
    void SetVisibility(bool bVisible);

protected:
	
    // --- Core ---
    void EnsureComponents(const UPatrolSystemSettings* Settings);
    void UpdateRouteVisuals(float DeltaSeconds);
    void RenderActiveRoutes(const UPatrolSystemSettings* Settings);
    void RenderRouteCached(const FPatrolRouteExtended& Route, int32 CacheIndex, const UPatrolSystemSettings* Settings);

    // --- Drawing Helpers ---
    void DrawSplinePolyline(const TArray<FVector>& Samples, const FLinearColor& Color, float Thickness, bool bDashed = false);
    void DrawDashedPolyline(const TArray<FVector>& Samples, const FLinearColor& Color, float Thickness);
    void DrawWaypoints(const TArray<FVector>& Points, const FLinearColor& Color, bool bShowNumbers, const FPatrolRouteExtended& Route, const UPatrolSystemSettings* Settings);
    void DrawDirectionArrows(const TArray<FVector>& Samples, const FLinearColor& Color, const UPatrolSystemSettings* Settings);

    // --- Geometry ---
    void BuildSplineSamples(const TArray<FVector>& Points, bool bLoop, TArray<FVector>& OutSamples, const UPatrolSystemSettings* Settings);
    static FVector EvaluateCatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T);
    static uint32 HashGeometry(const TArray<FVector>& Points, bool bLoop);
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

    UPROPERTY()
    TArray<FPatrolVisualizationCache> VisualizationCache;

    // --- Components ---
    UPROPERTY(Transient)
    TObjectPtr<ULineBatchComponent> LineBatchComponent = nullptr;

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