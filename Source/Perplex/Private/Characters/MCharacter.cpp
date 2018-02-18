// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#include "MCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UnrealNetwork.h"
#include "Characters/MCharacterMovementComponent.h"
#include "Characters/MPlayerController.h"
#include "Weapons/MWeapon.h"
#include "Weapons/MDamageType.h"

FMTakeHitInfo::FMTakeHitInfo()
	: ActualDamage(0.0f)
	, DamageTypeClass(nullptr)
	, PawnInstigator(nullptr)
	, DamageCauser(nullptr)
	, DamageEventClassID(0)
	, bKilled(false)
	, EnsureReplicationByte(0)
{
}

FDamageEvent& FMTakeHitInfo::GetDamageEvent()
{
	switch (DamageEventClassID)
	{
	case FPointDamageEvent::ClassID:
		if (!PointDamageEvent.DamageTypeClass)
		{
			PointDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
		}
		return PointDamageEvent;

	case FRadialDamageEvent::ClassID:
		if (!RadialDamageEvent.DamageTypeClass)
		{
			RadialDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
		}
		return RadialDamageEvent;

	default:
		if (!GeneralDamageEvent.DamageTypeClass)
		{
			GeneralDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
		}
		return GeneralDamageEvent;
	}
}

void FMTakeHitInfo::SetDamageEvent(const FDamageEvent& DamageEvent)
{
	DamageEventClassID = DamageEvent.GetTypeID();
	switch (DamageEventClassID)
	{
	case FPointDamageEvent::ClassID:
		PointDamageEvent = *((FPointDamageEvent const*)(&DamageEvent));
		break;
	case FRadialDamageEvent::ClassID:
		RadialDamageEvent = *((FRadialDamageEvent const*)(&DamageEvent));
		break;
	default:
		GeneralDamageEvent = DamageEvent;
	}

	DamageTypeClass = DamageEvent.DamageTypeClass;
}

void FMTakeHitInfo::EnsureReplication()
{
	EnsureReplicationByte++;
}

AMCharacter::AMCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	Capsule->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	CharacterMesh->bOnlyOwnerSee = false;
	CharacterMesh->bOwnerNoSee = true;
	CharacterMesh->bReceivesDecals = false;
	CharacterMesh->SetCollisionObjectType(ECC_Pawn);
	CharacterMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CharacterMesh->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	CharacterMesh->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	CharacterMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	MeshFP = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshFP"));
	MeshFP->SetupAttachment(GetCapsuleComponent());
	MeshFP->bOnlyOwnerSee = true;
	MeshFP->bOwnerNoSee = false;
	MeshFP->bCastDynamicShadow = false;
	MeshFP->bReceivesDecals = false;
	MeshFP->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	MeshFP->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	MeshFP->SetCollisionObjectType(ECC_Pawn);
	MeshFP->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshFP->SetCollisionResponseToAllChannels(ECR_Ignore);

	CharacterMovement = Cast<UMCharacterMovementComponent>(GetMovementComponent());

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
}

void AMCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Role == ROLE_Authority)
	{
		Health = 10.0f;
	}

	// set initial mesh visibility (3rd person view)
	UpdateCharacterMeshes();
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

void AMCharacter::Destroyed()
{
	Super::Destroyed();

	if (Role < ROLE_Authority)
	{
		return;
	}

	DestroyInventory();
}

void AMCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	UpdateCharacterMeshes();
	SetCurrentWeapon(CurrentWeapon);
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

void AMCharacter::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Only replicate this property for a short duration after it changes so join in progress players don't get spammed with fx when joining late
	DOREPLIFETIME_ACTIVE_OVERRIDE(AMCharacter, LastTakeHitInfo, GetWorld() && GetWorld()->GetTimeSeconds() < LastTakeHitTimeTimeout);
}

void AMCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// only to local owner: weapon change requests are locally instigated, other clients don't need it
	DOREPLIFETIME_CONDITION(AMCharacter, Inventory, COND_OwnerOnly);

	// everyone except local owner: flag change is locally instigated
	DOREPLIFETIME_CONDITION(AMCharacter, bIsAiming, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AMCharacter, bWantsToRun, COND_SkipOwner);

	DOREPLIFETIME_CONDITION(AMCharacter, LastTakeHitInfo, COND_Custom);

	// everyone
	DOREPLIFETIME(AMCharacter, CurrentWeapon);
	DOREPLIFETIME(AMCharacter, Health);
}

void AMCharacter::StartWeaponFire()
{
	if (!bWantsToFire)
	{
		bWantsToFire = true;
		if (IsRunning())
		{
			StopRun();
		}
		if (CurrentWeapon)
		{
			CurrentWeapon->StartFire();
		}
	}
}

void AMCharacter::StopWeaponFire()
{
	if (bWantsToFire)
	{
		bWantsToFire = false;
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
		}
	}
}

bool AMCharacter::CanFire() const
{
	return true;
}

void AMCharacter::StartAim()
{
	bIsAiming = true;
}

void AMCharacter::StopAim()
{
	bIsAiming = false;
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

void AMCharacter::StartRun()
{
	if (IsAiming())
	{
		SetAiming(false);
	}
	StopWeaponFire();
	SetRunning(true, false);
}

void AMCharacter::StartRunToggle()
{
	if (IsAiming())
	{
		SetAiming(false);
	}
	StopWeaponFire();
	SetRunning(true, true);
}

void AMCharacter::StopRun()
{
	SetRunning(false, false);
}

// Animations

float AMCharacter::PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName)
{
	USkeletalMeshComponent* UseMesh = GetCharacterMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance)
	{
		return UseMesh->AnimScriptInstance->Montage_Play(AnimMontage, InPlayRate);
	}

	return 0.0f;
}

void AMCharacter::StopAnimMontage(class UAnimMontage* AnimMontage)
{
	USkeletalMeshComponent* UseMesh = GetCharacterMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance &&
		UseMesh->AnimScriptInstance->Montage_IsPlaying(AnimMontage))
	{
		UseMesh->AnimScriptInstance->Montage_Stop(AnimMontage->BlendOut.GetBlendTime(), AnimMontage);
	}
}

void AMCharacter::StopAllAnimMontages()
{
	USkeletalMeshComponent* UseMesh = GetCharacterMesh();
	if (UseMesh && UseMesh->AnimScriptInstance)
	{
		UseMesh->AnimScriptInstance->Montage_Stop(0.0f);
	}
}

void AMCharacter::EquipWeapon(AMWeapon* Weapon)
{
	if (Weapon)
	{
		if (Role == ROLE_Authority)
		{
			SetCurrentWeapon(Weapon, CurrentWeapon);
		}
		else
		{
			ServerEquipWeapon(Weapon);
		}
	}
}

bool AMCharacter::ServerEquipWeapon_Validate(AMWeapon* Weapon)
{
	return true;
}

void AMCharacter::ServerEquipWeapon_Implementation(AMWeapon* Weapon)
{
	EquipWeapon(Weapon);
}

void AMCharacter::OnRep_CurrentWeapon(AMWeapon* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void AMCharacter::SetCurrentWeapon(AMWeapon* NewWeapon, AMWeapon* LastWeapon)
{
	AMWeapon* LocalLastWeapon = nullptr;

	if (LastWeapon != nullptr)
	{
		LocalLastWeapon = LastWeapon;
	}
	else if (NewWeapon != CurrentWeapon)
	{
		LocalLastWeapon = CurrentWeapon;
	}

	// unequip previous
	if (LocalLastWeapon)
	{
		LocalLastWeapon->OnUnEquip();
	}

	CurrentWeapon = NewWeapon;

	// equip new one
	if (NewWeapon)
	{
		NewWeapon->SetOwnerCharacter(this);	// Make sure weapon's MyPawn is pointing back to us. During replication, we can't guarantee APawn::CurrentWeapon will rep after AWeapon::MyPawn!

		NewWeapon->OnEquip(LastWeapon);
	}
}

void AMCharacter::DestroyInventory()
{
	if (Role < ROLE_Authority)
	{
		return;
	}

	for (int32 i = Inventory.Num() - 1; i >= 0; i--)
	{
		AMWeapon* Weapon = Inventory[i];
		if (Weapon)
		{
			Weapon->OnLeaveInventory();
			Inventory.RemoveSingle(Weapon);
			Weapon->Destroy();
		}
	}
}

float AMCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	if (Health <= 0.f)
	{
		return 0.f;
	}

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.f)
	{
		Health -= ActualDamage;
		if (Health <= 0)
		{
			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
		}
		else
		{
			//PlayHit(ActualDamage, DamageEvent, EventInstigator ? EventInstigator->GetPawn() : NULL, DamageCauser);
		}

		MakeNoise(1.0f, EventInstigator ? EventInstigator->GetPawn() : this);
	}

	return ActualDamage;
}

void AMCharacter::Suicide()
{
	KilledBy(this);
}

void AMCharacter::KilledBy(APawn* EventInstigator)
{
	if (Role == ROLE_Authority && !bIsDying)
	{
		AController* Killer = nullptr;
		if (EventInstigator != nullptr)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = nullptr;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, nullptr);
	}
}

bool AMCharacter::CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const
{
	if (bIsDying										// already dying
		|| IsPendingKill()								// already destroyed
		|| Role != ROLE_Authority						// not authority
		//|| GetWorld()->GetAuthGameMode<AShooterGameMode>() == NULL
		)//|| GetWorld()->GetAuthGameMode<AShooterGameMode>()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}

	return true;
}

bool AMCharacter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
	{
		return false;
	}

	Health = FMath::Min(0.0f, Health);

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (Controller != NULL) ? Controller : Cast<AController>(GetOwner());
	// TODO
	//GetWorld()->GetAuthGameMode<AShooterGameMode>()->Killed(Killer, KilledPlayer, this, DamageType);

	NetUpdateFrequency = GetDefault<AMCharacter>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : NULL, DamageCauser);
	return true;
}

void AMCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	Die(Health, FDamageEvent(dmgType.GetClass()), NULL, NULL);
}

void AMCharacter::OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (bIsDying)
	{
		return;
	}

	bReplicateMovement = false;
	bTearOff = true;
	bIsDying = true;

	if (Role == ROLE_Authority)
	{
		ReplicateHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);

		// play the force feedback effect on the client player controller
		AMPlayerController* PC = Cast<AMPlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UMDamageType *DamageType = Cast<UMDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			/*
			if (DamageType && DamageType->KilledForceFeedback && PC->IsVibrationEnabled())
			{
				PC->ClientPlayForceFeedback(DamageType->KilledForceFeedback, false, false, "Damage");
			}
			*/
		}
	}

	// remove all weapons
	DestroyInventory();

	// switch back to 3rd person view
	UpdateCharacterMeshes();

	DetachFromControllerPendingDestroy();
	StopAllAnimMontages();

	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}
	SetActorEnableCollision(true);

	// Death anim
	float TerminationAnimDuration = PlayAnimMontage(TerminationAnimation);

	// Ragdoll
	if (TerminationAnimDuration > 0.f)
	{
		// Trigger ragdoll a little before the animation early so the character doesn't
		// blend back to its normal position.
		const float TriggerRagdollTime = TerminationAnimDuration - 0.7f;

		// Enable blend physics so the bones are properly blending against the montage.
		GetMesh()->bBlendPhysics = true;

		// Use a local timer handle as we don't need to store it for later but we don't need to look for something to clear
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &AMCharacter::SetRagdollPhysics, FMath::Max(0.1f, TriggerRagdollTime), false);
	}
	else
	{
		SetRagdollPhysics();
	}

	// disable collisions on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void AMCharacter::UpdateCharacterMeshes()
{
	bool const bFirstPerson = IsFirstPerson();

	MeshFP->MeshComponentUpdateFlag = !bFirstPerson ? EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered : EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	MeshFP->SetOwnerNoSee(!bFirstPerson);

	GetMesh()->MeshComponentUpdateFlag = bFirstPerson ? EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered : EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	GetMesh()->SetOwnerNoSee(bFirstPerson);
}

void AMCharacter::TornOff()
{
	SetLifeSpan(25.f);
}

void AMCharacter::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsPendingKill())
	{
		bInRagdoll = false;
	}
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		bInRagdoll = false;
	}
	else
	{
		// initialize physics/etc
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan(1.0f);
	}
	else
	{
		SetLifeSpan(10.0f);
	}
}

void AMCharacter::ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const& LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
	if ((PawnInstigator == LastTakeHitInfo.PawnInstigator.Get()) && (LastDamageEvent.DamageTypeClass == LastTakeHitInfo.DamageTypeClass) && (LastTakeHitTimeTimeout == TimeoutTime))
	{
		// same frame damage
		if (bKilled && LastTakeHitInfo.bKilled)
		{
			// Redundant death take hit, just ignore it
			return;
		}

		// otherwise, accumulate damage done this frame
		Damage += LastTakeHitInfo.ActualDamage;
	}

	LastTakeHitInfo.ActualDamage = Damage;
	LastTakeHitInfo.PawnInstigator = Cast<AMCharacter>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();

	LastTakeHitTimeTimeout = TimeoutTime;
}

void AMCharacter::OnRep_LastTakeHitInfo()
{
	if (LastTakeHitInfo.bKilled)
	{
		OnDeath(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
	else
	{
		//PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
}

bool AMCharacter::IsMoving() const
{
	return FMath::Abs(GetLastMovementInputVector().Size()) > 0.f;
}

bool AMCharacter::IsRunning() const
{
	if (!CharacterMovement)
	{
		return false;
	}

	return (bWantsToRun || bWantsToRunToggled) && !GetVelocity().IsZero() && (GetVelocity().GetSafeNormal2D() | GetActorForwardVector()) > -0.1;
}

bool AMCharacter::IsFiring() const
{
	return bWantsToFire;
}

bool AMCharacter::IsAiming() const
{
	return bIsAiming;
}

AMWeapon* AMCharacter::GetWeapon() const
{
	return CurrentWeapon;
}

FRotator AMCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}

FVector AMCharacter::GetPawnViewLocation() const
{
	return GetActorLocation() + GetActorQuat().GetAxisZ() * BaseEyeHeight;
}

bool AMCharacter::IsEnemyFor(AController* TestController) const
{
	return true;
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

bool AMCharacter::IsFirstPerson() const
{
	//return IsAlive() && Controller && Controller->IsLocalPlayerController();
	return Controller && Controller->IsLocalPlayerController();
}

float AMCharacter::GetHealth() const
{
	return Health;
}

USkeletalMeshComponent* AMCharacter::GetCharacterMesh() const
{
	return IsFirstPerson() ? GetFirstPersonMesh() : GetThirdPersonMesh();
}

USkeletalMeshComponent* AMCharacter::GetFirstPersonMesh() const
{
	return MeshFP;
}

USkeletalMeshComponent* AMCharacter::GetThirdPersonMesh() const
{
	return GetMesh();
}

void AMCharacter::SetAiming(bool bNewAiming)
{
	bIsAiming = bNewAiming;

	if (Role < ROLE_Authority)
	{
		ServerSetAiming(bNewAiming);
	}
}

bool AMCharacter::ServerSetAiming_Validate(bool bNewAiming)
{
	return true;
}

void AMCharacter::ServerSetAiming_Implementation(bool bNewAiming)
{
	SetAiming(bNewAiming);
}

void AMCharacter::SetRunning(bool bNewRunning, bool bToggle)
{
	bWantsToRun = bNewRunning;
	bWantsToRunToggled = bNewRunning && bToggle;

	if (Role < ROLE_Authority)
	{
		ServerSetRunning(bNewRunning, bToggle);
	}
}

bool AMCharacter::ServerSetRunning_Validate(bool bNewRunning, bool bToggle)
{
	return true;
}

void AMCharacter::ServerSetRunning_Implementation(bool bNewRunning, bool bToggle)
{
	SetRunning(bNewRunning, bToggle);
}
