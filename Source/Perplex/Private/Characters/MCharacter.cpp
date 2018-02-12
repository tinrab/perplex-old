// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#include "MCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/MCharacterMovementComponent.h"
#include "Characters/MPlayerController.h"

AMCharacter::AMCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	CharacterMesh->bOnlyOwnerSee = false;
	CharacterMesh->bOwnerNoSee = true;
	CharacterMesh->bReceivesDecals = false;
	CharacterMesh->SetCollisionObjectType(ECC_Pawn);
	CharacterMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CharacterMesh->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	CharacterMesh->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	CharacterMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	Capsule->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	CharacterMovement = Cast<UMCharacterMovementComponent>(GetMovementComponent());

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
}

void AMCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CharacterMovement->SetGravityDirection(FVector(0.0f, 0.0f, -1.0f));

	if (bWantsToRunToggled && !IsRunning())
	{
		SetRunning(false, false);
	}
}

void AMCharacter::LaunchCharacterRotated(FVector LaunchVelocity, bool bHorizontalOverride, bool bVerticalOverride)
{
	UCharacterMovementComponent* CharacterMovement = GetCharacterMovement();
	if (CharacterMovement)
	{
		if (!bHorizontalOverride && !bVerticalOverride)
		{
			CharacterMovement->Launch(GetVelocity() + LaunchVelocity);
		}
		else if (bHorizontalOverride && bVerticalOverride)
		{
			CharacterMovement->Launch(LaunchVelocity);
		}
		else
		{
			FVector FinalVel;
			const FVector Velocity = GetVelocity();
			const FVector AxisZ = GetActorQuat().GetAxisZ();

			if (bHorizontalOverride)
			{
				FinalVel = FVector::VectorPlaneProject(LaunchVelocity, AxisZ) + AxisZ * (Velocity | AxisZ);
			}
			else // if (bVerticalOverride)
			{
				FinalVel = FVector::VectorPlaneProject(Velocity, AxisZ) + AxisZ * (LaunchVelocity | AxisZ);
			}

			CharacterMovement->Launch(FinalVel);
		}

		OnLaunched(LaunchVelocity, bHorizontalOverride, bVerticalOverride);
	}
}

void AMCharacter::ApplyDamageMomentum(float DamageTaken, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser)
{
	const UDamageType* DmgTypeCDO = DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>();
	const float ImpulseScale = DmgTypeCDO->DamageImpulse;

	UCharacterMovementComponent* CharacterMovement = GetCharacterMovement();
	if (ImpulseScale > 3.0f && CharacterMovement != nullptr)
	{
		FHitResult HitInfo;
		FVector ImpulseDir;
		DamageEvent.GetBestHitInfo(this, PawnInstigator, HitInfo, ImpulseDir);

		FVector Impulse = ImpulseDir * ImpulseScale;
		const bool bMassIndependentImpulse = !DmgTypeCDO->bScaleMomentumByMass;

		// Limit Z momentum added if already going up faster than jump (to avoid blowing character way up into the sky).
		{
			FVector MassScaledImpulse = Impulse;
			if (!bMassIndependentImpulse && CharacterMovement->Mass > SMALL_NUMBER)
			{
				MassScaledImpulse = MassScaledImpulse / CharacterMovement->Mass;
			}

			const FVector AxisZ = GetActorQuat().GetAxisZ();
			if ((CharacterMovement->Velocity | AxisZ) > GetDefault<UCharacterMovementComponent>(CharacterMovement->GetClass())->JumpZVelocity && (MassScaledImpulse | AxisZ) > 0.0f)
			{
				Impulse = FVector::VectorPlaneProject(Impulse, AxisZ) + AxisZ * ((Impulse | AxisZ) * 0.5f);
			}
		}

		CharacterMovement->AddImpulse(Impulse, bMassIndependentImpulse);
	}
}

FVector AMCharacter::GetPawnViewLocation() const
{
	return GetActorLocation() + GetActorQuat().GetAxisZ() * BaseEyeHeight;
}

void AMCharacter::PostNetReceiveLocationAndRotation()
{
	// Always consider Location as changed if we were spawned this tick as in that case our replicated Location was set as part of spawning, before PreNetReceive().
	if (ReplicatedMovement.Location == GetActorLocation() && ReplicatedMovement.Rotation == GetActorRotation() && CreationTime != GetWorld()->TimeSeconds)
	{
		return;
	}

	if (Role == ROLE_SimulatedProxy)
	{
		const FVector OldLocation = GetActorLocation();
		const FQuat OldRotation = GetActorQuat();
		const FQuat NewRotation = ReplicatedMovement.Rotation.Quaternion();

		// Correction to make sure pawn doesn't penetrate floor after replication rounding.
		ReplicatedMovement.Location += NewRotation.GetAxisZ() * 0.01f;

		SetActorLocationAndRotation(ReplicatedMovement.Location, ReplicatedMovement.Rotation, /*bSweep=*/ false);

		INetworkPredictionInterface* PredictionInterface = Cast<INetworkPredictionInterface>(GetMovementComponent());
		if (PredictionInterface)
		{
			PredictionInterface->SmoothCorrection(OldLocation, OldRotation, ReplicatedMovement.Location, NewRotation);
		}
	}
}

void AMCharacter::MoveHorizontal(float Amount)
{
	if (Amount != 0.0f)
	{
		FRotator ControlRotation = GetControlRotation();
		ControlRotation.Roll = 0.0f;
		ControlRotation.Pitch = 0.0f;
		const FQuat Rotation = GetActorQuat() * ControlRotation.Quaternion();

		AddMovementInput(Rotation.GetRightVector(), Amount);
	}
}

void AMCharacter::MoveVertical(float Amount)
{
	if (Amount != 0.0f)
	{
		FRotator ControlRotation = GetControlRotation();
		ControlRotation.Roll = 0.0f;
		ControlRotation.Pitch = 0.0f;
		const FQuat Rotation = GetActorQuat() * ControlRotation.Quaternion();

		AddMovementInput(Rotation.GetForwardVector(), Amount);
	}
}

bool AMCharacter::IsEnemyFor(AController* TestController) const
{
	return false;
	/*
	if (TestPC == Controller || TestPC == nullptr)
	{
		return false;
	}

	AShooterPlayerState* TestPlayerState = Cast<AShooterPlayerState>(TestPC->PlayerState);
	AShooterPlayerState* MyPlayerState = Cast<AShooterPlayerState>(PlayerState);

	bool bIsEnemy = true;
	if (GetWorld()->GetGameState())
	{
		const AShooterGameMode* DefGame = GetWorld()->GetGameState()->GetDefaultGameMode<AShooterGameMode>();
		if (DefGame && MyPlayerState && TestPlayerState)
		{
			bIsEnemy = DefGame->CanDealDamage(TestPlayerState, MyPlayerState);
		}
	}

	return bIsEnemy;
	*/
}

bool AMCharacter::IsMoving()
{
	return FMath::Abs(GetLastMovementInputVector().Size()) > 0.f;
}

void AMCharacter::SetRunning(bool bNewRunning, bool bToggle)
{
	bWantsToRun = bNewRunning;
	bWantsToRunToggled = bNewRunning && bToggle;
}

void AMCharacter::StartRun()
{
	AMPlayerController* MyPC = Cast<AMPlayerController>(Controller);
	if (MyPC)
	{
		SetRunning(true, false);
	}
}

void AMCharacter::StartRunToggle()
{
	AMPlayerController* MyPC = Cast<AMPlayerController>(Controller);
	if (MyPC)
	{
		SetRunning(true, true);
	}
}

void AMCharacter::StopRun()
{
	SetRunning(false, false);
}

bool AMCharacter::IsRunning() const
{
	if (!CharacterMovement)
	{
		return false;
	}

	return (bWantsToRun || bWantsToRunToggled) && !GetVelocity().IsZero() && (GetVelocity().GetSafeNormal2D() | GetActorForwardVector()) > -0.1;
}

void AMCharacter::StartJump()
{
	AMPlayerController* MyPC = Cast<AMPlayerController>(Controller);
	if (MyPC)
	{
		bPressedJump = true;
	}
}

void AMCharacter::StopJump()
{
	bPressedJump = false;
	StopJumping();
}
