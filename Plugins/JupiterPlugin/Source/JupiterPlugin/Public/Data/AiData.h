#pragma once

#include "CoreMinimal.h"
#include "AiData.generated.h"


UENUM(BlueprintType)
enum ECommandType
{
	CommandMove,
	CommandMoveFast,
	CommandMoveSlow,
	CommandAttack,
	CommandPatrol
};

UENUM(BlueprintType)
enum EFormation
{
	Line	UMETA(DisplayName = "Line Formation"),
	Column	UMETA(DisplayName = "Column Formation"),
	Wedge	UMETA(DisplayName = "Wedge Formation"),
	Blob	UMETA(DisplayName = "Blob Formation"),
	Square	UMETA(DisplayName = "Square Formation"),
};

UENUM(BlueprintType)
enum class ECombatBehavior : uint8
{
	Neutral	UMETA(DisplayName = "Neutral Combat Behavior"),
	Passive	UMETA(DisplayName = "Passive Combat Behavior"),
	Aggressive	UMETA(DisplayName = "Aggressive Combat Behavior")
};

UENUM(BlueprintType)
enum class ETeams : uint8
{
	Clone	UMETA(DisplayName = "Clone Team"),
	Droid	UMETA(DisplayName = "Droid Team"),
	HiveMind	UMETA(DisplayName = "HiveMind Team")
};

UENUM(BlueprintType)
enum class EAttackType : uint8
{
	Melee	UMETA(DisplayName = "Melee"),
	Ranged	UMETA(DisplayName = "Ranged")
};

USTRUCT(BlueprintType)
struct FAttackSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
	float PreferredRange = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
	float MaxRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
	float ChaseDistance = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
	float Cooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	EAttackType AttackType = EAttackType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	bool bUseWeaponRange = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0", EditCondition = "bUseWeaponRange"))
	float WeaponRangePadding = 50.f;

	float ResolveMaxRange(const float WeaponRange) const
	{
		const bool bHasWeaponRange = bUseWeaponRange && WeaponRange > 0.f;
		const float EffectiveMaxRange = bHasWeaponRange ? FMath::Max(WeaponRange - WeaponRangePadding, 0.f) : MaxRange;
		return FMath::Max(EffectiveMaxRange, PreferredRange);
	}

	float ResolvePreferredRange(const float WeaponRange) const
	{
		const float ClampedMax = ResolveMaxRange(WeaponRange);
		return FMath::Clamp(PreferredRange, MinRange, ClampedMax);
	}

	float ResolveMinRange(const float WeaponRange) const
	{
		const float ClampedMax = ResolveMaxRange(WeaponRange);
		return FMath::Clamp(MinRange, 0.f, ClampedMax);
	}

	float ResolveChaseDistance(const float WeaponRange) const
	{
		const float EffectiveMax = ResolveMaxRange(WeaponRange);
		return FMath::Max(ChaseDistance, EffectiveMax);
	}

	float ResolveCooldown() const
	{
		return FMath::Max(Cooldown, 0.f);
	}
};


USTRUCT(BlueprintType)
struct FCommandData
{
	GENERATED_BODY()

	// Initialisation
	FCommandData()
	:	RequestingController(nullptr),
		SourceLocation(FVector::ZeroVector),
		Location(FVector::ZeroVector),
		Rotation(FRotator::ZeroRotator),
		Type(ECommandType::CommandMove),
		Target(nullptr),
		Radius(0.f) {}

	// Assignation des parametres 
	FCommandData(APlayerController* InRequesting, const FVector InLocation, const FRotator InRotation, const ECommandType InType, AActor* InTarget = nullptr, const float InRadius = 0.0f)
	:	RequestingController(InRequesting),
		SourceLocation(InLocation),
		Location(InLocation),
		Rotation(InRotation),
		Type(InType),
		Target(InTarget),
		Radius(InRadius) {}


	// Variables
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	APlayerController* RequestingController;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector SourceLocation;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector Location;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FRotator Rotation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TEnumAsByte<ECommandType> Type;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	AActor* Target;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Radius;
};
