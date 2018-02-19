// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#include "MWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "UnrealNetwork.h"
#include "MCharacter.h"
#include "MPlayerCharacter.h"
#include "MPlayerController.h"

AMWeapon::AMWeapon()
{
	MeshFP = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	MeshFP->SetOnlyOwnerSee(true);
	MeshFP->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	MeshFP->SetReceivesDecals(false);
	MeshFP->SetCastShadow(false);
	MeshFP->SetCollisionObjectType(ECC_WorldDynamic);
	MeshFP->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshFP->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = MeshFP;

	MeshTP = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdPersonMesh"));
	MeshTP->SetOwnerNoSee(true);
	MeshTP->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	MeshTP->SetReceivesDecals(false);
	MeshTP->SetCastShadow(true);
	MeshTP->SetCollisionObjectType(ECC_WorldDynamic);
	MeshTP->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshTP->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshTP->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	MeshTP->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	MeshTP->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	MeshTP->SetupAttachment(MeshFP);

	bLoopedFireAnimation = false;
	bPlayingFireAnimation = false;
	bIsEquipped = false;
	bWantsToFire = false;
	bPendingEquip = false;
	CurrentState = EMWeaponState::Idle;
	BurstCounter = 0;
	LastFireTime = 0.0f;

	PrimaryActorTick.bCanEverTick = true;
	Super::SetTickGroup(TG_PrePhysics);
	Super::SetReplicates(true);
	bNetUseOwnerRelevancy = true;
}

USkeletalMeshComponent* AMWeapon::GetWeaponMesh() const
{
	return (OwnerCharacter != NULL && OwnerCharacter->IsFirstPerson()) ? MeshFP : MeshTP;
}

void AMWeapon::StartFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

void AMWeapon::StopFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStopFire();
	}

	if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

bool AMWeapon::CanFire() const
{
	/*
	bool bHasEnergy = Energy >= WeaponData.Consumption;
	bool bStateOKToFire = ((CurrentState == EMWeaponState::Idle) || (CurrentState == EMWeaponState::Firing));
	return bHasEnergy && bStateOKToFire;
	*/
	return true;
}

void AMWeapon::AddEnergy(float Amount)
{
	const float MissingEnergy = FMath::Max(0.0f, WeaponData.MaxEnergy - Energy);
	Amount = FMath::Min(Amount, MissingEnergy);
	Energy += Amount;
}

void AMWeapon::OnEquip(const AMWeapon* LastWeapon)
{
	AttachMeshToPawn();

	bPendingEquip = true;
	DetermineWeaponState();

	// Only play animation if last weapon is valid
	if (LastWeapon)
	{
		float Duration = PlayWeaponAnimation(EquipAnimation);
		if (Duration <= 0.0f)
		{
			// failsafe
			Duration = 0.5f;
		}
		EquipStartedTime = GetWorld()->GetTimeSeconds();
		EquipDuration = Duration;

		GetWorldTimerManager().SetTimer(TimerHandle_OnEquipFinished, this, &AMWeapon::OnEquipFinished, Duration, false);
	}
	else
	{
		OnEquipFinished();
	}
}

void AMWeapon::OnEquipFinished()
{
	AttachMeshToPawn();

	bIsEquipped = true;
	bPendingEquip = false;

	DetermineWeaponState();
}

void AMWeapon::OnUnEquip()
{
	DetachMeshFromPawn();
	bIsEquipped = false;
	StopFire();

	if (bPendingEquip)
	{
		StopWeaponAnimation(EquipAnimation);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_OnEquipFinished);
	}

	DetermineWeaponState();
}

void AMWeapon::OnEnterInventory(AMCharacter* NewOwner)
{
	SetOwnerCharacter(NewOwner);
}

void AMWeapon::OnLeaveInventory()
{
	if (Role == ROLE_Authority)
	{
		SetOwnerCharacter(nullptr);
	}

	if (IsAttachedToPawn())
	{
		OnUnEquip();
	}
}

void AMWeapon::ConsumeEnergy()
{
	Energy -= WeaponData.Consumption;
}

void AMWeapon::HandleFiring()
{
	if (CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
		{
			FireWeapon();

			ConsumeEnergy();

			// Update firing FX on remote clients if function was called on server
			BurstCounter++;
		}
	}
	else if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		/*
		if (GetCurrentAmmo() == 0 && !bRefiring)
		{
		PlayWeaponSound(OutOfAmmoSound);
		AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(MyPawn->Controller);
		AShooterHUD* MyHUD = MyPC ? Cast<AShooterHUD>(MyPC->GetHUD()) : NULL;
		if (MyHUD)
		{
		MyHUD->NotifyOutOfAmmo();
		}
		}
		*/

		// stop weapon fire FX, but stay in Firing state
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}

	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		// local client will notify server
		if (Role < ROLE_Authority)
		{
			ServerHandleFiring();
		}

		// setup refire timer
		bRefiring = (CurrentState == EMWeaponState::Firing && (1.0f / WeaponData.FireRate) > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AMWeapon::HandleFiring, 1.0f / WeaponData.FireRate, false);
		}
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
}

bool AMWeapon::ServerHandleFiring_Validate()
{
	return true;
}

void AMWeapon::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateEnergy = CanFire();

	HandleFiring();

	if (bShouldUpdateEnergy)
	{
		ConsumeEnergy();
		BurstCounter++;
	}
}

bool AMWeapon::ServerStartFire_Validate()
{
	return true;
}

void AMWeapon::ServerStartFire_Implementation()
{
	StartFire();
}

bool AMWeapon::ServerStopFire_Validate()
{
	return true;
}

void AMWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

void AMWeapon::OnRep_OwnerCharacter()
{
	if (OwnerCharacter)
	{
		OnEnterInventory(OwnerCharacter);
	}
	else
	{
		OnLeaveInventory();
	}
}

void AMWeapon::OnRep_BurstCounter()
{
	if (BurstCounter > 0)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}

void AMWeapon::SimulateWeaponFire()
{
	if (Role == ROLE_Authority && CurrentState != EMWeaponState::Firing)
	{
		return;
	}

	if (MuzzleEffect)
	{
		USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
		if (!bLoopedMuzzleEffect || MuzzleParticleSystem == NULL)
		{
			// Split screen requires we create 2 effects. One that we see and one that the other player sees.
			if ((OwnerCharacter != nullptr) && (OwnerCharacter->IsLocallyControlled() == true))
			{
				AController* PlayerCon = OwnerCharacter->GetController();
				if (PlayerCon != nullptr)
				{
					MeshFP->GetSocketLocation(MuzzleAttachPoint);
					MuzzleParticleSystem = UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshFP, MuzzleAttachPoint);
					MuzzleParticleSystem->bOwnerNoSee = false;
					MuzzleParticleSystem->bOnlyOwnerSee = true;

					MeshTP->GetSocketLocation(MuzzleAttachPoint);
					MuzzleParticleSystemSecondary = UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshTP, MuzzleAttachPoint);
					MuzzleParticleSystemSecondary->bOwnerNoSee = true;
					MuzzleParticleSystemSecondary->bOnlyOwnerSee = false;
				}
			}
			else
			{
				MuzzleParticleSystem = UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, UseWeaponMesh, MuzzleAttachPoint);
			}
		}
	}

	if (!bLoopedFireAnimation || !bPlayingFireAnimation)
	{
		PlayWeaponAnimation(FireAnimation);
		bPlayingFireAnimation = true;
	}

	AMPlayerController* PC = (OwnerCharacter != nullptr) ? Cast<AMPlayerController>(OwnerCharacter->Controller) : nullptr;
	if (PC && PC->IsLocalController())
	{
		if (FireCameraShake)
		{
			PC->ClientPlayCameraShake(FireCameraShake, 1);
		}
	}
}

void AMWeapon::StopSimulatingWeaponFire()
{
	if (bLoopedMuzzleEffect)
	{
		if (MuzzleParticleSystem)
		{
			MuzzleParticleSystem->DeactivateSystem();
			MuzzleParticleSystem = nullptr;
		}
		if (MuzzleParticleSystemSecondary)
		{
			MuzzleParticleSystemSecondary->DeactivateSystem();
			MuzzleParticleSystemSecondary = nullptr;
		}
	}

	if (bLoopedFireAnimation && bPlayingFireAnimation)
	{
		StopWeaponAnimation(FireAnimation);
		bPlayingFireAnimation = false;
	}
}

float AMWeapon::PlayWeaponAnimation(const FMWeaponAnimation& Animation)
{
	float Duration = 0.0f;
	if (OwnerCharacter)
	{
		UAnimMontage* UseAnim = OwnerCharacter->IsFirstPerson() ? Animation.FirstPerson : Animation.ThirdPerson;
		if (UseAnim)
		{
			Duration = OwnerCharacter->PlayAnimMontage(UseAnim);
		}
	}

	return Duration;
}

void AMWeapon::StopWeaponAnimation(const FMWeaponAnimation& Animation)
{
	if (OwnerCharacter)
	{
		UAnimMontage* UseAnim = OwnerCharacter->IsFirstPerson() ? Animation.FirstPerson : Animation.ThirdPerson;
		if (UseAnim)
		{
			OwnerCharacter->StopAnimMontage(UseAnim);
		}
	}
}

FVector AMWeapon::GetAdjustedAim() const
{
	AMPlayerController* const PC = Instigator ? Cast<AMPlayerController>(Instigator->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;

	// If we have a player controller use it for the aim
	if (PC)
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		// Now see if we have an AI controller - we will want to get the aim from there if we do
		// TODO
	}

	return FinalAim;
}

FVector AMWeapon::GetCameraAim() const
{
	AMPlayerController* const PC = Instigator ? Cast<AMPlayerController>(Instigator->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;

	if (PC)
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		FinalAim = Instigator->GetBaseAimRotation().Vector();
	}

	return FinalAim;
}

FVector AMWeapon::GetCameraDamageStartLocation(const FVector& AimDir) const
{
	AMPlayerController* PC = OwnerCharacter ? Cast<AMPlayerController>(OwnerCharacter->Controller) : nullptr;
	FVector OutStartTrace = FVector::ZeroVector;

	if (PC)
	{
		// Use player's camera
		FRotator UnusedRot;
		PC->GetPlayerViewPoint(OutStartTrace, UnusedRot);

		// Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
		OutStartTrace = OutStartTrace + AimDir * ((Instigator->GetActorLocation() - OutStartTrace) | AimDir);
	}

	return OutStartTrace;
}

FVector AMWeapon::GetMuzzleLocation() const
{
	USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketLocation(MuzzleAttachPoint);
}

FVector AMWeapon::GetMuzzleDirection() const
{
	USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketRotation(MuzzleAttachPoint).Vector();
}

FHitResult AMWeapon::WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const
{
	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(WeaponTrace), true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, TraceFrom, TraceTo, COLLISION_WEAPON, TraceParams);

	return Hit;
}

void AMWeapon::DetermineWeaponState()
{
	EMWeaponState NewState = EMWeaponState::Idle;

	if (bIsEquipped)
	{
		if (bWantsToFire && CanFire())
		{
			NewState = EMWeaponState::Firing;
		}
	}
	else if (bPendingEquip)
	{
		NewState = EMWeaponState::Equipping;
	}
}

void AMWeapon::SetWeaponState(EMWeaponState NewState)
{
	const EMWeaponState PrevState = CurrentState;

	if (PrevState == EMWeaponState::Firing && NewState != EMWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EMWeaponState::Firing && NewState == EMWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

void AMWeapon::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	const float TimeBetweenShots = 1.0f / WeaponData.FireRate;
	if (LastFireTime > 0 && TimeBetweenShots > 0.0f &&
		LastFireTime + TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AMWeapon::HandleFiring, LastFireTime + TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}

void AMWeapon::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
	bRefiring = false;
}

void AMWeapon::AttachMeshToPawn()
{
	if (OwnerCharacter)
	{
		// Remove and hide both first and third person meshes
		DetachMeshFromPawn();

		// For locally controller players we attach both weapons and let the bOnlyOwnerSee, bOwnerNoSee flags deal with visibility.
		FName AttachPoint = OwnerCharacter->GetWeaponAttachPoint();
		if (OwnerCharacter->IsLocallyControlled())
		{
			USkeletalMeshComponent* PawnMesh1p = OwnerCharacter->GetFirstPersonMesh();
			USkeletalMeshComponent* PawnMesh3p = OwnerCharacter->GetThirdPersonMesh();
			MeshFP->SetHiddenInGame(false);
			MeshTP->SetHiddenInGame(false);
			MeshFP->AttachToComponent(PawnMesh1p, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
			MeshTP->AttachToComponent(PawnMesh3p, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
		}
		else
		{
			USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
			USkeletalMeshComponent* UsePawnMesh = OwnerCharacter->GetCharacterMesh();
			UseWeaponMesh->AttachToComponent(UsePawnMesh, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
			UseWeaponMesh->SetHiddenInGame(false);
		}
	}
}

void AMWeapon::DetachMeshFromPawn()
{
	MeshFP->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	MeshFP->SetHiddenInGame(true);

	MeshTP->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	MeshTP->SetHiddenInGame(true);
}

void AMWeapon::SetOwnerCharacter(AMCharacter* NewOwner)
{
	if (OwnerCharacter != NewOwner)
	{
		Instigator = NewOwner;
		OwnerCharacter = NewOwner;
		SetOwner(NewOwner);
	}
}
