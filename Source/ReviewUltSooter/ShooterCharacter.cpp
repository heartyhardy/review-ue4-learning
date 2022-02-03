// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/AnimInstance.h"
#include "Particles/ParticleSystemComponent.h"


// Sets default values
AShooterCharacter::AShooterCharacter() :
	BaseTurnRate(45.f),
	BaseLookupRate(45.f)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->SocketOffset = FVector{ 0.f, 50.f, 50.f };

	//Enable Camra boom rotation based on the controller
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Disable controller Rotation for the Camera
	FollowCamera->bUsePawnControlRotation = false;

	/** These settings are temp until Strafing is implemented */

	// Don't orient character to camera
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = true;

	// Orient towards movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator{ 0.f, 540.f, 0.f };

	// Set Jump Velocity and Air control
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;
}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AShooterCharacter::MoveForward(float Value)
{
	if (Controller && Value != 0.f)
	{
		FRotator ControlRotation{ Controller->GetControlRotation() };
		FRotator YawRotation{ 0.f, ControlRotation.Yaw, 0.f };
		FVector ForwardDirection{ FRotationMatrix{ YawRotation }.GetUnitAxis(EAxis::X) };

		AddMovementInput(ForwardDirection, Value);
	}
}

void AShooterCharacter::MoveRight(float Value)
{
	if (Controller && Value != 0.f)
	{
		FRotator ControlRotation{ Controller->GetControlRotation() };
		FRotator YawRotation{ 0.f, ControlRotation.Yaw, 0.f };
		FVector ForwardDirection{ FRotationMatrix{ YawRotation }.GetUnitAxis(EAxis::Y) };

		AddMovementInput(ForwardDirection, Value);
	}
}

void AShooterCharacter::TurnRightAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookupRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::FireWeapon()
{
	if (GunshotsCue)
	{
		UGameplayStatics::PlaySound2D(this, GunshotsCue);
	}

	const USkeletalMeshSocket* MuzzleSocket = GetMesh()->GetSocketByName("WeaponLSocket");

	if (MuzzleSocket)
	{
		const FTransform SocketTransform{ MuzzleSocket->GetSocketTransform(GetMesh()) };

		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAttached(
				MuzzleFlash,
				RootComponent,
				NAME_None,
				SocketTransform.GetLocation() + FVector(0.f, 0.f, 25.f),
				SocketTransform.GetRotation().Rotator(),
				EAttachLocation::KeepWorldPosition
			);
		}

		FVector End;
		bool bWeaponHit = GetBeamEndLocation(SocketTransform.GetLocation(), End);

		if (bWeaponHit)
		{
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					End
				);
			}

			if (SmokeBeam)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					SmokeBeam,
					SocketTransform
				);

				if (Beam)
				{
					Beam->SetVectorParameter(FName("Target"), End);
				}
			}
		}
		/*
		// HitTest
		FHitResult FireHit;
		FVector Start{ SocketTransform.GetLocation() };
		FVector RotationAxis{ SocketTransform.GetRotation().GetAxisX() };
		FVector End{ Start + RotationAxis * 50'000.f };

		GetWorld()->LineTraceSingleByChannel(
			FireHit,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (FireHit.bBlockingHit)
		{
			End = FireHit.Location;

			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					FireHit.Location
				);
			}
		}

		if (SmokeBeam)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				SmokeBeam,
				Start
			);

			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), End);
			}
		}
		*/
	}

	UAnimInstance* ShooterAnimInstance = GetMesh()->GetAnimInstance();
	
	if (ShooterAnimInstance && FirePrimaryMontage)
	{
		ShooterAnimInstance->Montage_Play(FirePrimaryMontage);
	}
}

bool AShooterCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FVector& BeamEndLocation)
{
	FVector2D ViewportSize;

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation{ ViewportSize.X / 2.f, ViewportSize.Y / 2.f };
	CrosshairLocation.Y -= 50.f;

	FVector CrosshairWorldLocation;
	FVector CrosshairWorldDirection;

	bool bProjection = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldLocation,
		CrosshairWorldDirection
	);

	if (bProjection)
	{
		FHitResult FireResult;
		FVector Start{ CrosshairWorldLocation };
		BeamEndLocation = { CrosshairWorldLocation + CrosshairWorldDirection * 50'000.f };

		GetWorld()->LineTraceSingleByChannel(
			FireResult,
			Start,
			BeamEndLocation,
			ECollisionChannel::ECC_Visibility
		);

		if (FireResult.bBlockingHit)
		{
			BeamEndLocation = FireResult.Location;
		}

		// Second trace from gun barrel

		FHitResult BarrelTrace;
		FVector BarrelTraceStart{ MuzzleSocketLocation };
		FVector BarrelTraceEnd{ BeamEndLocation };

		GetWorld()->LineTraceSingleByChannel(
			BarrelTrace,
			BarrelTraceStart,
			BarrelTraceEnd,
			ECollisionChannel::ECC_Visibility
		);

		if (BarrelTrace.bBlockingHit)
		{
			BeamEndLocation = BarrelTrace.Location;
		}

		return true;
	}
	return false;
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);

	PlayerInputComponent->BindAxis("TurnRight", this, &AShooterCharacter::TurnRightAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUpAtRate);

	PlayerInputComponent->BindAxis("MouseTurn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("MouseLookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireWeapon", EInputEvent::IE_Pressed, this, &AShooterCharacter::FireWeapon);
}

