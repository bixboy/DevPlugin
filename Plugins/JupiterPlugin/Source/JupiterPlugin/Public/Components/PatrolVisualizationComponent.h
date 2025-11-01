#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "PatrolVisualizationComponent.generated.h"

/** Visualisation data for a single patrol route. */
USTRUCT()
struct FPatrolRouteVisualization
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FVector> Points;

    UPROPERTY()
    bool bIsLoop = false;

    UPROPERTY()
    FColor Color = FColor::White;

    bool operator==(const FPatrolRouteVisualization& Other) const
    {
        return bIsLoop == Other.bIsLoop && Color == Other.Color && Points == Other.Points;
    }

    bool operator!=(const FPatrolRouteVisualization& Other) const
    {
        return !(*this == Other);
    }
};

/** Component responsible for rendering patrol route visuals at runtime. */
UCLASS(ClassGroup = (RTS))
class JUPITERPLUGIN_API UPatrolVisualizationComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UPatrolVisualizationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // UPrimitiveComponent interface
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

    /** Updates the list of active patrol routes to render. */
    void SetActiveRoutes(const TArray<FPatrolRouteVisualization>& Routes);

    /** Clears all active patrol routes from the render list. */
    void ClearActiveRoutes();

    /** Updates the currently edited patrol route preview. */
    void SetPendingRoute(const TArray<FVector>& Points, bool bIsLoop, bool bHasPreview, const FVector& PreviewLocation, const FColor& Color);

    /** Clears the pending patrol route preview. */
    void ClearPendingRoute();

    /** Sets the line thickness used when drawing patrol segments. */
    void SetLineThickness(float Thickness);

    /** Sets the size used when drawing patrol nodes. */
    void SetPointSize(float Size);

    /** Sets the vertical offset applied to every rendered patrol point. */
    void SetHeightOffset(float Offset);

protected:
    /** Cached patrol routes visible in the current frame. */
    UPROPERTY(Transient)
    TArray<FPatrolRouteVisualization> ActiveRoutes;

    /** Patrol route currently being edited by the player. */
    UPROPERTY(Transient)
    TArray<FVector> PendingRoutePoints;

    /** Whether the pending route closes into a loop. */
    UPROPERTY(Transient)
    bool bPendingRouteIsLoop = false;

    /** Should a preview segment towards the cursor be rendered? */
    UPROPERTY(Transient)
    bool bPendingRouteHasPreview = false;

    /** Cursor world position used for the pending route preview. */
    UPROPERTY(Transient)
    FVector PendingPreviewLocation = FVector::ZeroVector;

    /** Colour applied to the pending patrol route. */
    UPROPERTY(Transient)
    FColor PendingRouteColor = FColor::Yellow;

    /** Thickness applied to all patrol segments. */
    UPROPERTY(EditAnywhere, Category = "RTS|Patrol")
    float RouteLineThickness = 3.f;

    /** Screen-space size applied to the rendered patrol nodes. */
    UPROPERTY(EditAnywhere, Category = "RTS|Patrol")
    float RoutePointSize = 12.f;

    /** Height offset applied to improve visibility above the terrain. */
    UPROPERTY(EditAnywhere, Category = "RTS|Patrol")
    float RouteHeightOffset = 20.f;

    /** True when the pending route has changed since the last render update. */
    bool bPendingRouteDirty = true;

    /** True when the active routes changed since the last render update. */
    bool bActiveRoutesDirty = true;

    /** Marks the render state dirty when visual data changes. */
    void RefreshRenderStateIfNeeded();
};

