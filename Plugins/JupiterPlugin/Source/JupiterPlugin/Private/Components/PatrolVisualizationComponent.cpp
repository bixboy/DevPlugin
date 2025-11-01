#include "Components/PatrolVisualizationComponent.h"

#include "PrimitiveSceneProxy.h"
#include "SceneManagement.h"

namespace
{
    /** Scene proxy responsible for submitting patrol route geometry to the renderer. */
    class FPatrolVisualizationSceneProxy final : public FPrimitiveSceneProxy
    {
    public:
        FPatrolVisualizationSceneProxy(const UPatrolVisualizationComponent* Component)
            : FPrimitiveSceneProxy(Component)
            , ActiveRoutes(Component->ActiveRoutes)
            , PendingRoutePoints(Component->PendingRoutePoints)
            , bPendingRouteIsLoop(Component->bPendingRouteIsLoop)
            , bPendingRouteHasPreview(Component->bPendingRouteHasPreview)
            , PendingPreviewLocation(Component->PendingPreviewLocation)
            , PendingRouteColor(Component->PendingRouteColor)
            , RouteLineThickness(Component->RouteLineThickness)
            , RoutePointSize(Component->RoutePointSize)
            , RouteHeightOffset(Component->RouteHeightOffset)
        {
            bWillEverBeLit = false;
        }

        virtual SIZE_T GetTypeHash() const override
        {
            static size_t UniquePointer;
            return reinterpret_cast<size_t>(&UniquePointer);
        }

        virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
        {
            const FVector VerticalOffset = FVector::UpVector * RouteHeightOffset;

            for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
            {
                if (!(VisibilityMap & (1 << ViewIndex)))
                {
                    continue;
                }

                FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

                for (const FPatrolRouteVisualization& Route : ActiveRoutes)
                {
                    DrawRoute(Route.Points, Route.Color, Route.bIsLoop, PDI, VerticalOffset);
                }

                if (PendingRoutePoints.Num() > 0)
                {
                    DrawRoute(PendingRoutePoints, PendingRouteColor, bPendingRouteIsLoop, PDI, VerticalOffset);

                    if (bPendingRouteHasPreview)
                    {
                        const FVector LastPoint = PendingRoutePoints.Last() + VerticalOffset;
                        const FVector PreviewPoint = PendingPreviewLocation + VerticalOffset;
                        PDI->DrawLine(LastPoint, PreviewPoint, PendingRouteColor, SDPG_World, RouteLineThickness, 0.f, true);
                        PDI->DrawPoint(PreviewPoint, PendingRouteColor, RoutePointSize, SDPG_World);
                    }
                }
            }
        }

        virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
        {
            FPrimitiveViewRelevance Result;
            Result.bDrawRelevance = true;
            Result.bDynamicRelevance = true;
            Result.bShadowRelevance = false;
            Result.bTranslucentSelfShadow = false;
            Result.bNormalTranslucencyRelevance = false;
            return Result;
        }

        virtual uint32 GetMemoryFootprint() const override
        {
            return sizeof(*this) + GetAllocatedSize();
        }

        uint32 GetAllocatedSize() const
        {
            return ActiveRoutes.GetAllocatedSize() + PendingRoutePoints.GetAllocatedSize();
        }

    private:
        void DrawRoute(const TArray<FVector>& Points, const FColor& Color, bool bLoop, FPrimitiveDrawInterface* PDI, const FVector& VerticalOffset) const
        {
            if (Points.Num() == 0)
            {
                return;
            }

            for (int32 PointIndex = 0; PointIndex < Points.Num(); ++PointIndex)
            {
                const FVector CurrentPoint = Points[PointIndex] + VerticalOffset;
                PDI->DrawPoint(CurrentPoint, Color, RoutePointSize, SDPG_World);

                const int32 NextIndex = PointIndex + 1;
                if (NextIndex < Points.Num())
                {
                    const FVector NextPoint = Points[NextIndex] + VerticalOffset;
                    PDI->DrawLine(CurrentPoint, NextPoint, Color, SDPG_World, RouteLineThickness, 0.f, true);
                }
            }

            if (bLoop && Points.Num() > 1)
            {
                const FVector FirstPoint = Points[0] + VerticalOffset;
                const FVector LastPoint = Points.Last() + VerticalOffset;
                PDI->DrawLine(LastPoint, FirstPoint, Color, SDPG_World, RouteLineThickness, 0.f, true);
            }
        }

        const TArray<FPatrolRouteVisualization> ActiveRoutes;
        const TArray<FVector> PendingRoutePoints;
        const bool bPendingRouteIsLoop;
        const bool bPendingRouteHasPreview;
        const FVector PendingPreviewLocation;
        const FColor PendingRouteColor;
        const float RouteLineThickness;
        const float RoutePointSize;
        const float RouteHeightOffset;
    };
}

UPatrolVisualizationComponent::UPatrolVisualizationComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetVisibility(true);
    bIsEditorOnly = false;
    bHiddenInGame = false;
    bUseAttachParentBound = true;
}

FPrimitiveSceneProxy* UPatrolVisualizationComponent::CreateSceneProxy()
{
    if (ActiveRoutes.Num() == 0 && PendingRoutePoints.Num() == 0)
    {
        return nullptr;
    }

    return new FPatrolVisualizationSceneProxy(this);
}

FBoxSphereBounds UPatrolVisualizationComponent::CalcBounds(const FTransform& LocalToWorld) const
{
    FBox BoundingBox(ForceInit);

    auto ExpandBoundsWithPoints = [&BoundingBox](const TArray<FVector>& Points)
    {
        for (const FVector& Point : Points)
        {
            BoundingBox += Point;
        }
    };

    for (const FPatrolRouteVisualization& Route : ActiveRoutes)
    {
        ExpandBoundsWithPoints(Route.Points);
    }

    ExpandBoundsWithPoints(PendingRoutePoints);

    if (bPendingRouteHasPreview)
    {
        BoundingBox += PendingPreviewLocation;
    }

    if (!BoundingBox.IsValid)
    {
        BoundingBox = FBox(FVector::ZeroVector, FVector::ZeroVector);
    }

    BoundingBox = BoundingBox.ExpandBy(RouteHeightOffset);

    return FBoxSphereBounds(BoundingBox).TransformBy(LocalToWorld);
}

void UPatrolVisualizationComponent::SetActiveRoutes(const TArray<FPatrolRouteVisualization>& Routes)
{
    if (ActiveRoutes != Routes)
    {
        ActiveRoutes = Routes;
        bActiveRoutesDirty = true;
        RefreshRenderStateIfNeeded();
    }
}

void UPatrolVisualizationComponent::ClearActiveRoutes()
{
    if (!ActiveRoutes.IsEmpty())
    {
        ActiveRoutes.Reset();
        bActiveRoutesDirty = true;
        RefreshRenderStateIfNeeded();
    }
}

void UPatrolVisualizationComponent::SetPendingRoute(const TArray<FVector>& Points, bool bIsLoop, bool bHasPreview, const FVector& PreviewLocation, const FColor& Color)
{
    const bool bPointsChanged = PendingRoutePoints != Points;
    const bool bLoopChanged = bPendingRouteIsLoop != bIsLoop;
    const bool bPreviewChanged = bPendingRouteHasPreview != bHasPreview || (!bHasPreview ? false : !PendingPreviewLocation.Equals(PreviewLocation));
    const bool bColorChanged = PendingRouteColor != Color;

    if (bPointsChanged || bLoopChanged || bPreviewChanged || bColorChanged)
    {
        PendingRoutePoints = Points;
        bPendingRouteIsLoop = bIsLoop;
        bPendingRouteHasPreview = bHasPreview;
        PendingPreviewLocation = PreviewLocation;
        PendingRouteColor = Color;
        bPendingRouteDirty = true;
        RefreshRenderStateIfNeeded();
    }
}

void UPatrolVisualizationComponent::ClearPendingRoute()
{
    if (!PendingRoutePoints.IsEmpty() || bPendingRouteHasPreview)
    {
        PendingRoutePoints.Reset();
        bPendingRouteIsLoop = false;
        bPendingRouteHasPreview = false;
        PendingPreviewLocation = FVector::ZeroVector;
        bPendingRouteDirty = true;
        RefreshRenderStateIfNeeded();
    }
}

void UPatrolVisualizationComponent::SetLineThickness(float Thickness)
{
    if (!FMath::IsNearlyEqual(RouteLineThickness, Thickness))
    {
        RouteLineThickness = Thickness;
        bPendingRouteDirty = true;
        bActiveRoutesDirty = true;
        RefreshRenderStateIfNeeded();
    }
}

void UPatrolVisualizationComponent::SetPointSize(float Size)
{
    if (!FMath::IsNearlyEqual(RoutePointSize, Size))
    {
        RoutePointSize = Size;
        bPendingRouteDirty = true;
        bActiveRoutesDirty = true;
        RefreshRenderStateIfNeeded();
    }
}

void UPatrolVisualizationComponent::SetHeightOffset(float Offset)
{
    if (!FMath::IsNearlyEqual(RouteHeightOffset, Offset))
    {
        RouteHeightOffset = Offset;
        bPendingRouteDirty = true;
        bActiveRoutesDirty = true;
        RefreshRenderStateIfNeeded();
    }
}

void UPatrolVisualizationComponent::RefreshRenderStateIfNeeded()
{
    if (bActiveRoutesDirty || bPendingRouteDirty)
    {
        bActiveRoutesDirty = false;
        bPendingRouteDirty = false;
        MarkRenderStateDirty();
    }
}

