// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UShooterAnimInstance::NativeInitializeAnimation()
{
	ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}

void UShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (!ShooterCharacter)
	{
		ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
	}

	if (ShooterCharacter)
	{
		FVector CharacterVelocity{ ShooterCharacter->GetVelocity() };
		CharacterVelocity.Z = 0.f; // We only need lateral velocity
		Speed = CharacterVelocity.Size();

		bIsInAir = ShooterCharacter->GetMovementComponent()->IsFalling();

		if (ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f)
		{
			bAccelerating = true;
		}
		else
		{
			bAccelerating = false;
		}
	}
}
