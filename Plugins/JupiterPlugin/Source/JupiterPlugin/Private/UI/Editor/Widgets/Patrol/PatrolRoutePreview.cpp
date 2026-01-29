#include "UI/Editor/Widgets/Patrol/PatrolRoutePreview.h"
#include "Components/Image.h"
#include "Rendering/DrawElements.h"

void UPatrolRoutePreview::NativeConstruct()
{
    Super::NativeConstruct();

    if (MapBackgroundImage)
    {
        MapMaterialInstance = MapBackgroundImage->GetDynamicMaterial();
        if (WorldMapTexture && MapMaterialInstance)
        {
            MapMaterialInstance->SetTextureParameterValue(FName("MapTexture"), WorldMapTexture);
        }
    }
}

FReply UPatrolRoutePreview::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        OnPreviewClicked.Broadcast();
        return FReply::Handled();
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPatrolRoutePreview::SetPatrolPoints(const TArray<FVector>& WorldPoints, bool bIsLoop)
{
	CachedPoints = WorldPoints;
    CachedIsLoop = bIsLoop;

	if (CachedPoints.Num() > 0)
	{
		BoundsMin = FVector2D(CachedPoints[0].X, CachedPoints[0].Y);
		BoundsMax = BoundsMin;

		for (const FVector& Pt : CachedPoints)
		{
			BoundsMin.X = FMath::Min(BoundsMin.X, Pt.X);
			BoundsMin.Y = FMath::Min(BoundsMin.Y, Pt.Y);
			BoundsMax.X = FMath::Max(BoundsMax.X, Pt.X);
			BoundsMax.Y = FMath::Max(BoundsMax.Y, Pt.Y);
		}

		if (FMath::IsNearlyEqual(BoundsMin.X, BoundsMax.X))
		{
			BoundsMin.X -= 500;
			BoundsMax.X += 500;
		}
		
		if (FMath::IsNearlyEqual(BoundsMin.Y, BoundsMax.Y))
		{
			BoundsMin.Y -= 500; 
			BoundsMax.Y += 500;
		}
	}

	Invalidate(EInvalidateWidgetReason::Paint);
}

void UPatrolRoutePreview::SetMapTexture(UTexture2D* Texture, FVector2D MapWorldSize, FVector2D MapWorldOrigin)
{
    WorldMapTexture = Texture;
    WorldMapSize = MapWorldSize;
    WorldMapOrigin = MapWorldOrigin;

    if (MapMaterialInstance)
    {
        MapMaterialInstance->SetTextureParameterValue(FName("MapTexture"), WorldMapTexture);
    }
}

int32 UPatrolRoutePreview::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    UpdateMapZoom(AllottedGeometry);

	int32 MaxLayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (CachedPoints.Num() < 2)
		return MaxLayerId;

	const FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	TArray<FVector2D> DrawPoints;

	for (const FVector& WorldPt : CachedPoints)
	{
		DrawPoints.Add(WorldToWidget(WorldPt, WidgetSize));
	}
	
	if (CachedPoints.Num() > 2 && CachedIsLoop)
	{
		const FVector2D FirstPoint = DrawPoints[0];
		DrawPoints.Add(FirstPoint); 
	}

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(),
		DrawPoints,
		ESlateDrawEffect::None,
		PathColor,
		true,
		LineThickness
	);

	for (const FVector2D& Pt : DrawPoints)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2D(PointSize, PointSize), FSlateLayoutTransform(Pt - (PointSize * 0.5f))),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			PathColor
		);
	}

	return LayerId + 2;
}

void UPatrolRoutePreview::UpdateMapZoom(const FGeometry& AllottedGeometry) const
{
	if (!MapMaterialInstance || CachedPoints.Num() == 0)
 		return;
	
    float ContentWidth = BoundsMax.Y - BoundsMin.Y;
    float ContentHeight = BoundsMax.X - BoundsMin.X;
    
    ContentWidth = FMath::Max(ContentWidth, 100.0f); 
    ContentHeight = FMath::Max(ContentHeight, 100.0f);

    float ContentAspect = ContentWidth / ContentHeight;

    FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    float WidgetAspect = LocalSize.X / FMath::Max(LocalSize.Y, 1.0f);
	
    ViewBoundsMin = BoundsMin;
    ViewBoundsMax = BoundsMax;

    if (WidgetAspect > ContentAspect)
    {
        float TargetWidth = ContentHeight * WidgetAspect;
        float Diff = TargetWidth - ContentWidth;
        ViewBoundsMin.Y -= Diff * 0.5f;
        ViewBoundsMax.Y += Diff * 0.5f;
    }
    else
    {
        float TargetHeight = ContentWidth / WidgetAspect;
        float Diff = TargetHeight - ContentHeight;
        ViewBoundsMin.X -= Diff * 0.5f;
        ViewBoundsMax.X += Diff * 0.5f;
    }

    float ViewSizeY = ViewBoundsMax.Y - ViewBoundsMin.Y;
    float ViewSizeX = ViewBoundsMax.X - ViewBoundsMin.X;
    
    const float UV_ScaleX = ViewSizeY / WorldMapSize.Y;
    const float UV_ScaleY = ViewSizeX / WorldMapSize.X;

    const float UV_OffsetX = (ViewBoundsMin.Y - WorldMapOrigin.Y) / WorldMapSize.Y;
    const float UV_OffsetY = (WorldMapOrigin.X - ViewBoundsMax.X) / WorldMapSize.X;

    MapMaterialInstance->SetScalarParameterValue(FName("UVScaleX"), UV_ScaleX);
    MapMaterialInstance->SetScalarParameterValue(FName("UVScaleY"), UV_ScaleY);
    MapMaterialInstance->SetScalarParameterValue(FName("UVOffsetX"), UV_OffsetX);
    MapMaterialInstance->SetScalarParameterValue(FName("UVOffsetY"), UV_OffsetY);
}

FVector2D UPatrolRoutePreview::WorldToWidget(FVector WorldPos, FVector2D WidgetSize) const
{
	float ViewWidthWorld = ViewBoundsMax.Y - ViewBoundsMin.Y;
	float ViewHeightWorld = ViewBoundsMax.X - ViewBoundsMin.X;
	
	float AlphaX = (WorldPos.Y - ViewBoundsMin.Y) / ViewWidthWorld;
	float AlphaY = 1.0f - ((WorldPos.X - ViewBoundsMin.X) / ViewHeightWorld);

	return FVector2D(AlphaX * WidgetSize.X, AlphaY * WidgetSize.Y);
}