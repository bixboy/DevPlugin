#include "Utils/JupiterInputHelpers.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"

namespace JupiterInputHelpers
{
    bool IsKeyDown(const APlayerController* Player, const FKey& Key)
    {
        return Player ? Player->IsInputKeyDown(Key) : false;
    }

    bool WasKeyJustPressed(const APlayerController* Player, const FKey& Key)
    {
        return Player ? Player->WasInputKeyJustPressed(Key) : false;
    }

    float GetKeyTimeDown(const APlayerController* Player, const FKey& Key)
    {
        return Player ? Player->GetInputKeyTimeDown(Key) : 0.f;
    }

    bool IsAltDown(const APlayerController* Player)
    {
        if (!Player)
        {
            return false;
        }

        return Player->IsInputKeyDown(EKeys::LeftAlt) || Player->IsInputKeyDown(EKeys::RightAlt);
    }

    bool TryGetMousePosition(const APlayerController* Player, FVector2D& OutPosition)
    {
        if (!Player)
        {
            return false;
        }

        float LocationX = 0.f;
        float LocationY = 0.f;
        const bool bHasPosition = Player->GetMousePosition(LocationX, LocationY);
        if (bHasPosition)
        {
            OutPosition = FVector2D(LocationX, LocationY);
        }

        return bHasPosition;
    }

    bool TryGetNormalizedMousePosition(const APlayerController* Player, FVector2D& OutNormalizedPosition)
    {
        FVector2D MousePosition;
        if (!TryGetMousePosition(Player, MousePosition))
        {
            return false;
        }

        int32 ViewportX = 0;
        int32 ViewportY = 0;
        if (!Player || !Player->GetViewportSize(ViewportX, ViewportY) || ViewportX == 0 || ViewportY == 0)
        {
            return false;
        }

        const float ScaleX = 1.f / static_cast<float>(ViewportX);
        const float ScaleY = 1.f / static_cast<float>(ViewportY);
        OutNormalizedPosition = FVector2D(MousePosition.X * ScaleX, MousePosition.Y * ScaleY);
        return true;
    }
}

