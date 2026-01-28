#include "UI/JupiterToggleSwitch.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"

// SJupiterToggleSwitch Implementation

void SJupiterToggleSwitch::Construct(const FArguments& InArgs)
{
	IsToggled = InArgs._IsToggled;
	TintColor = InArgs._TintColor;
	InactiveColor = InArgs._InactiveColor;
	ThumbColor = InArgs._ThumbColor;
	SwitchSize = InArgs._SwitchSize;
	OnToggled = InArgs._OnToggled;

	CurrentThumbPosition = IsToggled.Get() ? 1.0f : 0.0f;
	TargetThumbPosition = CurrentThumbPosition;

	// Initialize Brushes for Rounded Look
	BackgroundBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	BackgroundBrush.OutlineSettings.CornerRadii = FVector4(14.0f, 14.0f, 14.0f, 14.0f); // Will be updated in OnPaint
	BackgroundBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	// Using WhiteBrush as the resource to allow tinting
	BackgroundBrush.SetResourceObject(FCoreStyle::Get().GetBrush("WhiteBrush")->GetResourceObject());

	ThumbBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	ThumbBrush.OutlineSettings.CornerRadii = FVector4(14.0f, 14.0f, 14.0f, 14.0f);
	ThumbBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	ThumbBrush.SetResourceObject(FCoreStyle::Get().GetBrush("WhiteBrush")->GetResourceObject());
}

int32 SJupiterToggleSwitch::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float Scale = Size.Y / 30.0f; // Reference height

	// Update corner radii to matches height for pill shape
	float CornerRadius = Size.Y * 0.5f;
	
	// Create a mutable copy of the brush to update runtime properties if needed
	FSlateBrush LocalBgBrush = BackgroundBrush;
	LocalBgBrush.OutlineSettings.CornerRadii = FVector4(CornerRadius, CornerRadius, CornerRadius, CornerRadius);

	FSlateBrush LocalThumbBrush = ThumbBrush;
	float ThumbRadius = (Size.Y * 0.5f) - (Size.Y * 0.1f); // Reduce by padding
	LocalThumbBrush.OutlineSettings.CornerRadii = FVector4(ThumbRadius, ThumbRadius, ThumbRadius, ThumbRadius);

    FLinearColor CurrentTintColor = TintColor.Get();
    FLinearColor CurrentInactiveColor = InactiveColor.Get();
    
    // Smooth color transition
    FLinearColor DrawColor = FMath::Lerp(CurrentInactiveColor, CurrentTintColor, CurrentThumbPosition);
    
    // Draw Background
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        &LocalBgBrush,
        ESlateDrawEffect::None,
        DrawColor
    );

    // Turn off tint for thumb
    // Draw Thumb (Circle)
    float ThumbPadding = Size.Y * 0.1f;
    float ThumbDiam = Size.Y - (ThumbPadding * 2.0f);
    float TrackWidth = Size.X - (ThumbPadding * 2.0f) - ThumbDiam;
    
    float ThumbX = ThumbPadding + (TrackWidth * CurrentThumbPosition);
    float ThumbY = ThumbPadding;
    
    FVector2D ThumbPos(ThumbX, ThumbY);
    FVector2D ThumbSize(ThumbDiam, ThumbDiam);
    
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId + 1,
        AllottedGeometry.ToPaintGeometry(ThumbSize, FSlateLayoutTransform(ThumbPos)),
        &LocalThumbBrush,
        ESlateDrawEffect::None,
        ThumbColor.Get()
    );

	return LayerId + 2;
}

FVector2D SJupiterToggleSwitch::ComputeDesiredSize(float) const
{
	return SwitchSize.Get();
}

FReply SJupiterToggleSwitch::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bool bNewState = !IsToggled.Get();
		// If attribute is bound, this Set might assume we are driving it. 
		// If it's a bound attribute (delegate), 'SetIsToggled' on the SWidget might not propagate up unless we execute a callback.
		// We execute the callback.
		
		OnToggled.ExecuteIfBound(bNewState);
		
		// If not bound (local state), update strictly? 
		// Actually, standard pattern: user updates state in response to delegate.
		// But for UMG wrapper properties (which are usually just values), we might need to eagerly update or wait for next frame.
		// For animation responsiveness, we update target immediately locally? 
		// But if the user logic denies the toggle, it would glitch.
		// Let's assume user accepts.
		
		// However, for UMG 'IsToggled', the UWidget needs to hear about it.
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SJupiterToggleSwitch::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    // Update Target based on state
    bool bCurrentState = IsToggled.Get();
    TargetThumbPosition = bCurrentState ? 1.0f : 0.0f;
    
	if (!FMath::IsNearlyEqual(CurrentThumbPosition, TargetThumbPosition, 0.01f))
	{
		CurrentThumbPosition = FMath::FInterpTo(CurrentThumbPosition, TargetThumbPosition, InDeltaTime, 15.0f);
	}
	else
	{
		CurrentThumbPosition = TargetThumbPosition;
	}
}

void SJupiterToggleSwitch::SetIsToggled(bool bToggled)
{
	IsToggled.Set(bToggled);
}

void SJupiterToggleSwitch::SetTintColor(FLinearColor InColor)
{
	TintColor.Set(InColor);
}

void SJupiterToggleSwitch::SetInactiveColor(FLinearColor InColor)
{
	InactiveColor.Set(InColor);
}

void SJupiterToggleSwitch::SetThumbColor(FLinearColor InColor)
{
	ThumbColor.Set(InColor);
}

void SJupiterToggleSwitch::SetSwitchSize(FVector2D InSize)
{
	SwitchSize.Set(InSize);
}

// UJupiterToggleSwitch Implementation

TSharedRef<SWidget> UJupiterToggleSwitch::RebuildWidget()
{
	MyToggleSwitch = SNew(SJupiterToggleSwitch)
		.IsToggled(bIsToggled)
		.TintColor(TintColor)
		.InactiveColor(InactiveColor)
		.ThumbColor(ThumbColor)
		.SwitchSize(SwitchSize)
		.OnToggled_UObject(this, &UJupiterToggleSwitch::HandleOnToggled);

	return MyToggleSwitch.ToSharedRef();
}

void UJupiterToggleSwitch::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MyToggleSwitch.IsValid())
	{
		MyToggleSwitch->SetIsToggled(bIsToggled);
		MyToggleSwitch->SetTintColor(TintColor);
		MyToggleSwitch->SetInactiveColor(InactiveColor);
		MyToggleSwitch->SetThumbColor(ThumbColor);
		MyToggleSwitch->SetSwitchSize(SwitchSize);
	}
}

void UJupiterToggleSwitch::ReleaseSlateResources(bool bReleaseChildren)
{
	MyToggleSwitch.Reset();
	Super::ReleaseSlateResources(bReleaseChildren);
}

void UJupiterToggleSwitch::SetIsToggled(bool bNewState)
{
	if (bIsToggled != bNewState)
	{
		bIsToggled = bNewState;
		if (MyToggleSwitch.IsValid())
		{
			MyToggleSwitch->SetIsToggled(bNewState);
		}
	}
}

void UJupiterToggleSwitch::HandleOnToggled(bool bNewState)
{
    // Called from Slate when user clicks
	if (bIsToggled != bNewState)
	{
		bIsToggled = bNewState;
		OnToggled.Broadcast(bIsToggled);
		// Force update slate to ensure animation triggers if it was waiting for update
		if (MyToggleSwitch.IsValid())
		{
			MyToggleSwitch->SetIsToggled(bNewState);
		}
	}
}
