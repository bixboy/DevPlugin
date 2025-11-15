#pragma once

#include "CoreMinimal.h"

class APlayerController;

namespace JupiterInputHelpers
{
    /** Returns true when the provided key is currently pressed. */
    JUPITERPLUGIN_API bool IsKeyDown(const APlayerController* Player, const FKey& Key);

    /** Returns true on the frame where the key transitioned from up to down. */
    JUPITERPLUGIN_API bool WasKeyJustPressed(const APlayerController* Player, const FKey& Key);

    /** Returns the amount of time the key has been held. */
    JUPITERPLUGIN_API float GetKeyTimeDown(const APlayerController* Player, const FKey& Key);

    /** Convenience wrapper around the ALT modifier state. */
    JUPITERPLUGIN_API bool IsAltDown(const APlayerController* Player);

    /** Retrieves the mouse position in viewport coordinates. */
    JUPITERPLUGIN_API bool TryGetMousePosition(const APlayerController* Player, FVector2D& OutPosition);

    /** Retrieves the mouse position normalized to [0,1] space. */
    JUPITERPLUGIN_API bool TryGetNormalizedMousePosition(const APlayerController* Player, FVector2D& OutNormalizedPosition);
}

