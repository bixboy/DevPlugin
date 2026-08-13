#pragma once
#include "CoreMinimal.h"
#include "FirstPersonBuilderState.generated.h"


/**
 * Step-based transform authoring: pin a transform, then refine it in discrete increments.
 *
 * PRESENTLY UNUSED. This was the Jupiter placement system's Place/Rotate/Offset cycle, removed
 * from that path in favour of the world-space gizmo. It is kept because the state machine itself
 * is sound and self-contained, and the same paradigm is wanted for first-person building later.
 *
 * Nothing includes this header today. Including it is therefore an explicit decision, which is
 * why it lives on its own rather than inside PlacementTypes.h where it would be pulled in by
 * everything that touches placement.
 */
/**
 * How the placement preview responds to input while an item is armed.
 *
 * Place is the free mode: the ghost tracks the cursor and snapping applies. The other modes pin
 * the position that Place resolved and let the Game Master refine it without the cursor moving
 * it again — which is the whole point, since any mouse twitch would otherwise undo the work.
 */
UENUM(BlueprintType)
enum class EFirstPersonBuilderMode : uint8
{
	/** Ghost follows the cursor, snapping active. */
	Place UMETA(DisplayName = "Place (Free)"),

	/** Position pinned; input turns the prop in fixed increments. */
	Rotate UMETA(DisplayName = "Rotate"),

	/** Position pinned; input nudges it by small offsets, including height. */
	Offset UMETA(DisplayName = "Offset (Precise)")
};


/**
 * Transform authoring state machine: pin a transform, then refine it in discrete steps.
 *
 * A plain USTRUCT held by value, deliberately not a component. There is nothing here that needs
 * an actor, a tick, registration or replication — it is a handful of fields and some arithmetic.
 * Embedding it costs only its own bytes, so a system can carry one per tool without the
 * per-instance overhead a UActorComponent would impose. Mirrors how FRotationPreviewState is
 * already used by the placement system.
 *
 * Owners keep the input bindings and change notifications; this only owns the state.
 */
USTRUCT()
struct FFirstPersonBuilderState
{
	GENERATED_BODY()

	/** Current mode. Place means nothing is pinned and the owner drives the transform freely. */
	UPROPERTY()
	EFirstPersonBuilderMode Mode = EFirstPersonBuilderMode::Place;

	/** Transform captured on leaving Place, held steady while the other modes refine it. */
	UPROPERTY()
	FVector PinnedLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator PinnedRotation = FRotator::ZeroRotator;

	/** World-space fine offset accumulated in Offset mode. */
	UPROPERTY()
	FVector ManualOffset = FVector::ZeroVector;

	/** Timestamp of the last accepted step, for the repeat gate. */
	double LastStepTime = 0.0;

	bool IsPinned() const { return Mode != EFirstPersonBuilderMode::Place; }

	/** Resolved transform: the pin plus any accumulated offset. */
	FVector GetLocation() const { return PinnedLocation + ManualOffset; }
	FRotator GetRotation() const { return PinnedRotation; }

	/** Back to free placement, pin and offset discarded. */
	void Reset()
	{
		Mode = EFirstPersonBuilderMode::Place;
		PinnedLocation = FVector::ZeroVector;
		PinnedRotation = FRotator::ZeroRotator;
		ManualOffset = FVector::ZeroVector;
		LastStepTime = 0.0;
	}

	void ClearOffset() { ManualOffset = FVector::ZeroVector; }

	/**
	 * Switches mode, capturing the given transform when entering a pinned mode.
	 *
	 * Returns true when the mode actually changed, so the owner only broadcasts on real changes.
	 */
	bool SetMode(EFirstPersonBuilderMode NewMode, const FVector& CurrentLocation, const FRotator& CurrentRotation)
	{
		if (Mode == NewMode)
			return false;

		const bool bWasPinned = IsPinned();
		Mode = NewMode;

		if (IsPinned() && !bWasPinned)
		{
			PinnedLocation = CurrentLocation;
			PinnedRotation = CurrentRotation;
			ManualOffset = FVector::ZeroVector;
		}
		else if (!IsPinned())
		{
			ManualOffset = FVector::ZeroVector;
		}

		return true;
	}

	/** Place -> Rotate -> Offset -> Place. */
	bool CycleMode(const FVector& CurrentLocation, const FRotator& CurrentRotation)
	{
		switch (Mode)
		{
		case EFirstPersonBuilderMode::Place:  return SetMode(EFirstPersonBuilderMode::Rotate, CurrentLocation, CurrentRotation);
		case EFirstPersonBuilderMode::Rotate: return SetMode(EFirstPersonBuilderMode::Offset, CurrentLocation, CurrentRotation);
		default:                        return SetMode(EFirstPersonBuilderMode::Place,  CurrentLocation, CurrentRotation);
		}
	}

	/**
	 * Rejects repeat input while an axis is held, so one keypress is one step.
	 *
	 * Enhanced Input fires Triggered every frame for a held axis; without this a nudge runs away
	 * at frame rate.
	 */
	bool ConsumeStep(double Now, double MinDelay)
	{
		if (Now - LastStepTime < MinDelay)
			return false;

		LastStepTime = Now;
		return true;
	}

	/**
	 * Turns the pinned yaw by StepDegrees * Multiplier.
	 *
	 * Multiplier is a count, not a sign: held input passes +/-1, while a caller that wants several
	 * increments at once passes the number of steps.
	 */
	void StepRotation(float StepDegrees, double Multiplier)
	{
		if (FMath::IsNearlyZero(Multiplier) || FMath::IsNearlyZero(StepDegrees))
			return;

		PinnedRotation.Yaw = FRotator::NormalizeAxis(PinnedRotation.Yaw + StepDegrees * Multiplier);
	}

	/**
	 * Adds WorldSteps whole increments of StepSize to the offset, per world axis.
	 *
	 * World axes, not a view-relative basis: an offset built while facing one way must still mean
	 * the same thing after the camera turns, otherwise a half-finished adjustment stops matching
	 * the keys that produced it.
	 *
	 * Components are counts, so the caller decides whether held input means one step or an
	 * explicit call means several.
	 */
	void AddOffset(const FVector& WorldSteps, float StepSize)
	{
		ManualOffset += WorldSteps * StepSize;
	}
};

