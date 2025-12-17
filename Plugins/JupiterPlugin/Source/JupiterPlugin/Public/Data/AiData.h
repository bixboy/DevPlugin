#pragma once
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
		Radius(0.f),
		PatrolPath(),
		bPatrolLoop(false),
		PatrolID(),
		StartIndex(0) {}

	// Assignation des parametres 
	FCommandData(APlayerController* InRequesting, const FVector InLocation, const FRotator InRotation, const ECommandType InType, AActor* InTarget = nullptr, const float InRadius = 0.0f)
	:	RequestingController(InRequesting),
		SourceLocation(InLocation),
		Location(InLocation),
		Rotation(InRotation),
		Type(InType),
		Target(InTarget),
		Radius(InRadius),
		PatrolPath(),
		bPatrolLoop(false),
		PatrolID(),
		StartIndex(0) {}


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

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FVector> PatrolPath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bPatrolLoop;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGuid PatrolID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 StartIndex;
};
