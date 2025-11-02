#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitPatrolComponent.generated.h"

class ULineBatchComponent;

/** Structure représentant une route de patrouille (répliquée). */
USTRUCT(BlueprintType)
struct FPatrolRoute
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector> PatrolPoints;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsLoop = false;
};

/** Cache local de rendu spline pour chaque route active. */
USTRUCT()
struct FPatrolRouteVisCache
{
    GENERATED_BODY()

    TArray<FVector> ElevatedPoints;
    TArray<FVector> SplineSamples;
    uint32 GeometryHash = 0;
    bool bLoopCached = false;
    FColor BaseColor = FColor::White;

    void Reset()
    {
        ElevatedPoints.Reset();
        SplineSamples.Reset();
        GeometryHash = 0;
        bLoopCached = false;
        BaseColor = FColor::White;
    }
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

protected:
    // === Internal Helpers ===
    void EnsureLineBatchComponent();
    void UpdateRouteVisuals(float DeltaSeconds);
    void DrawActiveRouteVisualizations();
    void DrawRouteVisualizationCached(const TArray<FVector>& Points, bool bLoop, const FColor& BaseColor, bool bAnimate, FPatrolRouteVisCache& Cache);

    void DrawPolylineWithGlow(const TArray<FVector>& Samples, const FLinearColor& CoreColor, float BaseThickness, bool bAnimate) const;
    void DrawPointsHalo(const TArray<FVector>& Points, const FColor& BaseColor) const;
    void DrawDirectionArrows(const TArray<FVector>& Samples, const FColor& Color) const;

    void BuildSplineSamples(const TArray<FVector>& Points, bool bLoop, TArray<FVector>& OutSamples) const;
    static FVector EvaluateCatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T);
    static uint32 HashPointsLight(const TArray<FVector>& Points, bool bLoop);

    bool IsLocallyControlled() const;
    bool HasAuthority() const;

    // === Data ===
    UPROPERTY(ReplicatedUsing=OnRep_ActivePatrolRoutes)
    TArray<FPatrolRoute> ActivePatrolRoutes;

    UFUNCTION() void OnRep_ActivePatrolRoutes();

    UPROPERTY(Transient) TObjectPtr<ULineBatchComponent> LineBatchComponent = nullptr;
    UPROPERTY() TArray<FPatrolRouteVisCache> RoutesVisCache;

    float GlobalVisualTime = 0.f;
    float VisualRefreshTimer = 0.f;
    bool bVisualsDirty = true;

    // === Settings ===
    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX")
    FColor ActiveRouteColor = FColor::Cyan;

    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX")
    float VisualZOffset = 12.f;

    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX")
    bool bAnimateActiveRoutes = true;

    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX")
    float VisualRefreshInterval = 0.05f;

    // Pulsations dynamiques
    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX") float ColorPulseSpeed = 2.4f;
    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX") float LineWaveSpeed = 2.6f;
    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX") float LineWaveAmplitude = 1.6f;
    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX") float PointPulseSpeed = 2.1f;

    // Glow
    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX") int32 GlowPasses = 2;
    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX") float GlowSize = 6.f;
    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX") float GlowAlpha = 0.25f;

    // Arrows
    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX") float ArrowEveryUU = 600.f;

    // Style
    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX") float DebugPointRadius = 40.f;
    UPROPERTY(EditAnywhere, Category="RTS|Patrol|VFX") float DebugLineThickness = 3.f;
};
