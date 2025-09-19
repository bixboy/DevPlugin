#include "Widgets/MouseSteeringShip.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"


void UMouseSteeringShip::NativeConstruct()
{
	Super::NativeConstruct();
	
	UpdateMouseVisuals();
}

void UMouseSteeringShip::SetupWidget(float NewNeutralThreshold, float NewMaxThreshold)
{
	NeutralThreshold = NewNeutralThreshold;
	MaxThreshold = NewMaxThreshold;
	
	UpdateMouseVisuals();
}


void UMouseSteeringShip::UpdateMouseVisuals()
{
	 APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    int32 ViewportX, ViewportY;
    PC->GetViewportSize(ViewportX, ViewportY);
    const float HalfMin = FMath::Min(ViewportX, ViewportY) * 0.5f;

    FVector2D ScreenCenter(ViewportX * 0.5f, ViewportY * 0.5f);

    float MouseX, MouseY;
    if (!PC->GetMousePosition(MouseX, MouseY))
        return;

    FVector2D MousePos(MouseX, MouseY);
    FVector2D Offset = MousePos - ScreenCenter;
    float Distance = Offset.Size();
    float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X));

    // --- Cercle neutre ---
    if (NeutralZoneImage)
    {
        float RadiusN = NeutralThreshold * HalfMin;
        FVector2D SizeN(RadiusN * 2.f, RadiusN * 2.f);

        if (auto* TempSlot = Cast<UCanvasPanelSlot>(NeutralZoneImage->Slot))
        {
            TempSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
            TempSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            TempSlot->SetSize(SizeN);
            TempSlot->SetPosition(FVector2D::ZeroVector);
        }
    }

    // --- Cercle max ---
    if (MaxZoneImage)
    {
        float RadiusM = MaxThreshold * HalfMin;
        FVector2D SizeM(RadiusM * 2.f, RadiusM * 2.f);

        if (auto* TempSlot = Cast<UCanvasPanelSlot>(MaxZoneImage->Slot))
        {
            TempSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
            TempSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            TempSlot->SetSize(SizeM);
            TempSlot->SetPosition(FVector2D::ZeroVector);
        }
    }

    // --- Ligne de direction ---
    if (MouseLineImage)
    {
        float ClampedDist = FMath::Min(Distance, MaxThreshold * HalfMin);
        FVector2D SizeL(ClampedDist, 2.f);

        if (auto* TempSlot = Cast<UCanvasPanelSlot>(MouseLineImage->Slot))
        {
            TempSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
            TempSlot->SetAlignment(FVector2D(0.f, 0.5f));
            TempSlot->SetSize(SizeL);
            TempSlot->SetPosition(FVector2D::ZeroVector);
        }

        MouseLineImage->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
        MouseLineImage->SetRenderTranslation(FVector2D::ZeroVector);
        MouseLineImage->SetRenderTransformAngle(AngleDeg);
    }
}


