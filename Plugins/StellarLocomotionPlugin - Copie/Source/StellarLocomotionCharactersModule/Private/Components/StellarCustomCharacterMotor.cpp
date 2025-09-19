// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/StellarCustomCharacterMotor.h"

#include "Components/CapsuleComponent.h"
#include "Interfaces/StellarCharacterController.h"
#include "Interfaces/CharacterInterface.h"



void UStellarCustomCharacterMotor::Tick(const float& DeltaTime)
{
	Super::Tick(DeltaTime);

	// Your tick logic here
	if (_owner->Implements<UCharacterInterface>())
	{
		//GEngine->AddOnScreenDebugMessage(-1,1.f, FColor::Green,FString::SanitizeFloat(DeltaTime));

		ICharacterInterface::Execute_StellarUpdateRotation(_owner, _rotation, DeltaTime);
		ICharacterInterface::Execute_StellarUpdateVelocity(_owner, _gravityVelocity, _movementVelocity, DeltaTime);
	}
	// if (_owner->Implements<UCharacterInterface>())
	// {
	// 	//GEngine->AddOnScreenDebugMessage(-1,1.f, FColor::Green,FString::SanitizeFloat(DeltaTime));
	// }
	
	UpdateTransform(DeltaTime);
	UpdateRotation(DeltaTime);
	UpdateMovements(DeltaTime);
	ApplyTransform(DeltaTime);
}


void UStellarCustomCharacterMotor::SetCharacterEnvironement(AActor* env)
{
	Super::SetCharacterEnvironement(env);
	
	InitialRelativeTransform = Environement ? _owner->GetActorTransform().GetRelativeTransform(Environement->GetActorTransform()) : FTransform::Identity;

	FString Msg = Environement ? "Environement is Valid" : "Environement is Invalid"; 
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
}

void UStellarCustomCharacterMotor::UpdateTransform(float DeltaTime)
{
	if (!Environement) return;
	
	const FTransform& ParentWorldTransform = Environement->GetActorTransform();
	
	SimulatedChildTransform = InitialRelativeTransform * ParentWorldTransform;
	FinalTransform = LocalOffsetTransform * SimulatedChildTransform;
}

void UStellarCustomCharacterMotor::UpdateRotation(float DeltaTime)
{
	FRotator DeltaRot(_rotation);
	FRotator ActorRotation = Environement ? LocalOffsetTransform.GetRotation().Rotator() : _owner->GetActorRotation();
	
	ActorRotation.Yaw += DeltaRot.Yaw;
	
	LocalOffsetTransform.SetRotation(ActorRotation.Quaternion());
	CurrentOffsetTransform.SetRotation(ActorRotation.Quaternion());
	_rotation = FQuat::Identity;
}

void UStellarCustomCharacterMotor::UpdateMovements(float DeltaTime)
{
	//FVector locomotionVelocity = _gravityVelocity;
	//locomotionVelocity = CollideAndSlide(locomotionVelocity, locomotionVelocity, _owner->GetActorLocation(), 0, false);
	
	// if(_isGrounded)
	// {
	// 	_finalVelocity = CollideAndSlide(_movementVelocity, _movementVelocity, _owner->GetActorLocation(), 0, false);
	// }else
	// {
	// 	_movementVelocity = CollideAndSlide(_movementVelocity, _movementVelocity, _owner->GetActorLocation(), 0, false);
	// 	_gravityVelocity = CollideAndSlide(_gravityVelocity, _gravityVelocity, _owner->GetActorLocation(), 0, true);
	// 	_finalVelocity = _gravityVelocity + _movementVelocity;
	// }
	//
	// _owner->SetActorLocation(_owner->GetActorLocation() + _finalVelocity);

	
	// _movementVelocity = CollideAndSlide(_movementVelocity, _movementVelocity, _owner->GetActorLocation(), 0, false);
	// _gravityVelocity = CollideAndSlide(_gravityVelocity, _gravityVelocity, _owner->GetActorLocation(), 0, true);
	//
	// _finalVelocity = _gravityVelocity + _movementVelocity;
	// _owner->SetActorLocation(_owner->GetActorLocation() + _movementVelocity);


	//_movementVelocity = HomeMadeCollideAndSlide(_movementVelocity, _movementVelocity, _owner->GetActorLocation(), 0, false);
	//_movementVelocity += HomeMadeCollideAndSlide(_gravityVelocity, _gravityVelocity, _owner->GetActorLocation() + _movementVelocity, 0, true);
	
	//_owner->SetActorLocation(_owner->GetActorLocation() + _movementVelocity);

	if(Environement)
	{
		FVector EndMovementVector = LocalOffsetTransform.GetLocation();
		
		_movementVelocity = HomeMadeCollideAndSlide(_movementVelocity, _movementVelocity, FinalTransform.GetLocation(), 0, false);
		//_movementVelocity += HomeMadeCollideAndSlide(_gravityVelocity, _gravityVelocity, FinalTransform.GetLocation() + _movementVelocity, 0, true);

		EndMovementVector += _movementVelocity;
		LocalOffsetTransform.SetLocation(EndMovementVector);
	}else
	{
		FVector EndMovementVector = _owner->GetActorLocation();
	
		_movementVelocity = HomeMadeCollideAndSlide(_movementVelocity, _movementVelocity, _owner->GetActorLocation(), 0, false);
		_movementVelocity += HomeMadeCollideAndSlide(_gravityVelocity, _gravityVelocity, _owner->GetActorLocation() + _movementVelocity, 0, true);

		EndMovementVector += _movementVelocity;
		CurrentOffsetTransform.SetLocation(EndMovementVector);
	}
	
}

void UStellarCustomCharacterMotor::ApplyTransform(float DeltaTime)
{
	if (Environement)
	{
		FinalTransform = LocalOffsetTransform * SimulatedChildTransform;
	
		// Apply user movement & yaw offset on top
		_owner->SetActorTransform(FinalTransform);
	}
	else
	{
		_owner->SetActorTransform(CurrentOffsetTransform);
	}
}

FVector UStellarCustomCharacterMotor::GetGlobalForwardVector()
{
	return Environement ? FinalTransform.GetUnitAxis(EAxis::X) : _owner->GetActorForwardVector();
}

FVector UStellarCustomCharacterMotor::GetGlobalRightVector()
{
	return Environement ? FinalTransform.GetUnitAxis(EAxis::Y) : _owner->GetActorRightVector();
}

FVector UStellarCustomCharacterMotor::GetGlobalUpVector()
{
	return Environement ? FinalTransform.GetUnitAxis(EAxis::Z) : _owner->GetActorUpVector();
}

FVector UStellarCustomCharacterMotor::GetCharacterForwardVector()
{
	return Environement ? LocalOffsetTransform.GetUnitAxis(EAxis::X) : _owner->GetActorForwardVector();
}

FVector UStellarCustomCharacterMotor::GetCharacterRightVector()
{
	return Environement ? LocalOffsetTransform.GetUnitAxis(EAxis::Y) : _owner->GetActorRightVector();
}

FVector UStellarCustomCharacterMotor::GetCharacterUpVector()
{
	return Environement ? LocalOffsetTransform.GetUnitAxis(EAxis::Z) : _owner->GetActorUpVector();
}



void UStellarCustomCharacterMotor::Dev_UpdateMovement(FVector MovementInput)
{
}



// FVector UStellarCharacterMotor::HomeMadeCollideAndSlide(FVector vel, FVector initialVel, FVector pos, int depth, bool gravityPass)
// {
// 	if (depth >= _maxBounces)
// 		return FVector::ZeroVector;
//
// 	const float dist = vel.Length() + _skinWidth;
//
// 	FVector Start = pos;
// 	FVector End = pos + vel.GetSafeNormal() * dist;
// 	const float CapsuleRadius = _bounds->GetUnscaledCapsuleRadius();
// 	const float CapsuleHalfHeight = _bounds->GetUnscaledCapsuleHalfHeight();
// 	const FQuat CapsuleRotation = _bounds->GetComponentRotation().Quaternion();
//
// 	FHitResult HitResult = CheckCapsuleCollision(Start, End, CapsuleRadius, CapsuleHalfHeight, CapsuleRotation, false);
//
// 	if (HitResult.bBlockingHit)
// 	{
// 		// Snap slightly above the surface to avoid jitter
// 		FVector snapToSurface = vel.GetSafeNormal() * (FMath::Max(0.f, HitResult.Distance - _skinWidth));
// 		FVector velocityLeftOver = vel - snapToSurface;
//
// 		if (snapToSurface.IsNearlyZero(0.01f))
// 		{
// 			snapToSurface = FVector::ZeroVector;
// 		}
//
// 		// Compute slope
// 		const float Dot = FVector::DotProduct(GetGlobalUpVector(), HitResult.Normal);
// 		const float angleRadians = FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f));
// 		const float angleDegree = FMath::RadiansToDegrees(angleRadians);
//
// 		// Ground & Slope
// 		if (angleDegree <= _maxSlopeAngle)
// 		{
// 			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Ground & Slope"));
//
// 			if (gravityPass)
// 			{
// 				// Land cleanly: stop vertical velocity
// 				return snapToSurface;
// 			}
//
// 			velocityLeftOver = ProjectAndScale(velocityLeftOver, HitResult.Normal);
// 		}
// 		else //Wall & Step
// 		{
// 			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Wall & Step"));
//
// 			// Check small upward sweep to allow stepping
// 			FHitResult StepHit;
// 			FVector StepStart = pos + FVector(0, 0, _maxStepHeight);
// 			FVector StepEnd = StepStart + vel.GetSafeNormal() * dist;
//
// 			bool bStepPossible = GetWorld()->SweepSingleByChannel(
// 				StepHit,
// 				StepStart,
// 				StepEnd,
// 				CapsuleRotation,
// 				ECC_Visibility,
// 				FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight),
// 				FCollisionQueryParams(FName(TEXT("StepTest")), false, _owner)
// 			);
//
// 			//Avoid climbing on wall
// 			const float SlopeCos = FVector::DotProduct(StepHit.Normal, FVector::UpVector);
// 			bool IsWalkable =  SlopeCos >= FMath::Cos(FMath::DegreesToRadians(_maxSlopeAngle));
// 			
// 			if (!bStepPossible && !IsWalkable)
// 			{
// 				// Step up instead of sliding against wall
// 				return FVector(0, 0, _maxStepHeight) + vel;
// 			}
//
// 			// Otherwise, slide along wall
// 			float scale = 1 - FVector::DotProduct(
// 				FVector(HitResult.Normal.X, HitResult.Normal.Y, 0).GetSafeNormal(),
// 				-FVector(initialVel.X, initialVel.Y, 0).GetSafeNormal()
// 			);
//
// 			velocityLeftOver = ProjectAndScale(velocityLeftOver, HitResult.Normal) * scale;
// 		}
//
// 		return snapToSurface + HomeMadeCollideAndSlide(velocityLeftOver, initialVel, pos + snapToSurface, depth + 1, gravityPass);
// 	}else
// 	{
// 		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Falling"));
// 	}
//
// 	return vel;
// }


bool UStellarCustomCharacterMotor::IsCharacterGrounded()
{
	const FVector Start = Environement ? FinalTransform.GetLocation() : _owner->GetActorLocation();
	const FVector DownVector = -GetGlobalUpVector();
	const FVector End = Start + DownVector * 3.0f; // Use a configurable value instead of _skinWidth * 3

	const float CapsuleRadius = _bounds->GetUnscaledCapsuleRadius();
	const float CapsuleHalfHeight = _bounds->GetUnscaledCapsuleHalfHeight();
	const FQuat CapsuleRotation = Environement ? FinalTransform.GetRotation() : _owner->GetActorQuat();

	FHitResult HitResult = CheckCapsuleCollision(Start, End, CapsuleRadius, CapsuleHalfHeight, CapsuleRotation, true);

	if (HitResult.bBlockingHit)
	{
		// Optionally add slope check:
		const float SlopeDot = FVector::DotProduct(HitResult.Normal, GetGlobalUpVector());
		const float SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(SlopeDot, -1.f, 1.f)));

		UE_LOG(LogTemp, Log, TEXT("Ground Hit Angle: %.2f degrees"), SlopeAngle);

		return SlopeAngle <= _maxSlopeAngle; // e.g., 45.f
	}

	return false;
}


FVector UStellarCustomCharacterMotor::HomeMadeCollideAndSlide(FVector vel, FVector initialVel, FVector pos, int depth, bool gravityPass)
{
	if(depth >= _maxBounces) {
		return FVector::ZeroVector;
	}
	
	float dist = vel.Length() + _skinWidth;

	FVector Start = pos;
	FVector End = pos + vel.GetSafeNormal() * dist;
	float CapsuleRadius = _bounds->GetUnscaledCapsuleRadius();
	float CapsuleHalfHeight = _bounds->GetUnscaledCapsuleHalfHeight();
	FQuat CapsuleRotation = _bounds->GetComponentRotation().Quaternion();
	FHitResult HitResult = CheckCapsuleCollision(Start, End, CapsuleRadius, CapsuleHalfHeight, CapsuleRotation, false);

	if(HitResult.bBlockingHit)
	{
		FVector snapToSurface = vel.GetSafeNormal() * (HitResult.Distance - _skinWidth);
		FVector velocityLeftOver = vel - snapToSurface;

		//Si le movement jusqu'au point de collision est inférieur à la distance de sécurité
		//Alors on annule le mouvement.
		if(snapToSurface.Length() <= _skinWidth) {
			snapToSurface = FVector::ZeroVector;
		}

		//Calcule l'angle en degrés et en radian de la collision
		float Dot = FVector::DotProduct(GetGlobalUpVector(), HitResult.Normal);
		float angleRadians = FMath::Acos(Dot);
		float angleDegree = FMath::RadiansToDegrees(angleRadians);

		//normal ground or slope
		if(angleDegree <= _maxSlopeAngle) {
			if(gravityPass) {
				return snapToSurface;
			}
			velocityLeftOver = ProjectAndScale(velocityLeftOver, HitResult.Normal);
		}
		// wall or steep slope
		else
		{
			float scale = 1 - FVector::DotProduct
			(
				FVector(HitResult.Normal.X, HitResult.Normal.Y, 0).GetSafeNormal(),
				-FVector(initialVel.X, initialVel.Y, 0).GetSafeNormal()
			);
			
			if(_isGrounded && !gravityPass)
			{
				velocityLeftOver = ProjectAndScale
				(
					FVector(velocityLeftOver.X, velocityLeftOver.Y, 0),
					FVector(HitResult.Normal.X, HitResult.Normal.Y, 0)
				).GetSafeNormal();
			
				velocityLeftOver *= scale;
			}else
			{
				velocityLeftOver = ProjectAndScale(velocityLeftOver, HitResult.Normal) * scale;
			}
		}
		
		return snapToSurface + HomeMadeCollideAndSlide(velocityLeftOver, initialVel, pos + snapToSurface, depth + 1, gravityPass);
	}

	 
	return vel;
}

FVector UStellarCustomCharacterMotor::ProjectAndScale(FVector vec, FVector normal)
{
	float mag = vec.Length();
	vec = FVector::VectorPlaneProject(vec, normal).GetSafeNormal();
	vec *= mag;
	
	return vec;
}




FVector UStellarCustomCharacterMotor::CollideAndSlide(FVector vel, FVector initialVel, FVector pos, int depth, bool gravityPass)
{
	if(depth >= _maxBounces) {
		return FVector::ZeroVector;
	}

	
	float dist = vel.Length() + _skinWidth;

	FVector Start = pos;
	FVector End = pos + vel.GetSafeNormal() * dist;
	float CapsuleRadius = _bounds->GetUnscaledCapsuleRadius();
	float CapsuleHalfHeight = _bounds->GetUnscaledCapsuleHalfHeight();
	FQuat CapsuleRotation = _bounds->GetComponentRotation().Quaternion();
	FHitResult HitResult = CheckCapsuleCollision(Start, End, CapsuleRadius, CapsuleHalfHeight, CapsuleRotation, false);

	if(HitResult.bBlockingHit)
	{
		FVector snapToSurface = vel.GetSafeNormal() * (HitResult.Distance - _skinWidth);
		FVector leftOver = vel - snapToSurface;
		
		float Dot = FVector::DotProduct(FVector::UpVector, HitResult.Normal);
		float angleRadians = FMath::Acos(Dot);
		float angleDegree = FMath::RadiansToDegrees(angleRadians);

		
		if(snapToSurface.Length() <= _skinWidth) {
			snapToSurface = FVector::ZeroVector;
		}

		//normal ground or slope
		if(angleDegree <= _maxSlopeAngle) {
			if(gravityPass) {
				return snapToSurface;
			}
			leftOver = ProjectAndScale(leftOver, HitResult.Normal);
		}
		// wall or steep slope
		else
		{
			float scale = 1 - FVector::DotProduct
			(
				FVector(HitResult.Normal.X, HitResult.Normal.Y, 0).GetSafeNormal(),
				-FVector(initialVel.X, initialVel.Y, 0).GetSafeNormal()
			);
			
			if(_isGrounded && !gravityPass)
			{
				leftOver = ProjectAndScale
				(
					FVector(leftOver.X, leftOver.Y, 0),
					FVector(HitResult.Normal.X, HitResult.Normal.Y, 0)
				).GetSafeNormal();
			
				leftOver *= scale;
			}else
			{
				leftOver = ProjectAndScale(leftOver, HitResult.Normal) * scale;
			}
		}

		// float mag = leftOver.Length();
		// leftOver = FVector::VectorPlaneProject(leftOver, HitResult.Normal).GetSafeNormal();
		// leftOver *= mag;
		
		return snapToSurface + CollideAndSlide(leftOver, initialVel, pos + snapToSurface, depth + 1, gravityPass);
	}

	 
	return vel;
}