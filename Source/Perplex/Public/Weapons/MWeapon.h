// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#pragma once

#include "Perplex.h"
#include "GameFramework/Actor.h"
#include "MWeapon.generated.h"

class AMCharacter;
class USkeletalMeshComponent;
class UAnimMontage;
class UParticleSystemComponent;
class UParticleSystem;

USTRUCT()
struct FMWeaponAnimation
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* FirstPerson;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ThirdPerson;
};

UENUM()
enum class EMWeaponState : uint8
{
	Idle,
	Firing,
	Equipping
};

USTRUCT()
struct FMWeaponData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Data")
	float MaxEnergy;

	UPROPERTY(EditDefaultsOnly, Category = "Data")
	float Consumption;

	UPROPERTY(EditDefaultsOnly, Category = "Data")
	float FireRate;
};

UCLASS(Abstract)
class PERPLEX_API AMWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AMWeapon();

	/** Return FP or TP weapon mesh */
	USkeletalMeshComponent* GetWeaponMesh() const;

	/** Start weapon fire */
	virtual void StartFire();

	/** Stop weapon fire */
	virtual void StopFire();

	/** Check if weapon can fire */
	bool CanFire() const;

	/** Return current weapon state */
	FORCEINLINE EMWeaponState GetCurrentState() const { return CurrentState; }

	/** Return current energy */
	FORCEINLINE float GetCurrentEnergy() const { return Energy; }

	/** Return weapon data */
	FORCEINLINE FMWeaponData GetWeaponData() const { return WeaponData; }

	/** Adds energy */
	void AddEnergy(float Amount);

	/** Weapon is being equipped by owner pawn */
	virtual void OnEquip(const AMWeapon* LastWeapon);

	/** Weapon is now equipped by owner pawn */
	virtual void OnEquipFinished();

	/** Weapon is holstered by owner pawn */
	virtual void OnUnEquip();

	/** Weapon was added to pawn's inventory */
	virtual void OnEnterInventory(AMCharacter* NewOwner);

	/** Weapon was removed from pawn's inventory */
	virtual void OnLeaveInventory();

	/** Check if it's currently equipped */
	FORCEINLINE bool IsEquipped() const { return bIsEquipped; };

	/** Check if mesh is already attached */
	FORCEINLINE bool IsAttachedToPawn() const { return bIsEquipped || bPendingEquip; };

	/** Set new owner */
	void SetOwnerCharacter(AMCharacter* NewOwner);

protected:
	/** Weapon specific fire implementation */
	virtual void FireWeapon() PURE_VIRTUAL(AMWeapon::FireWeapon(), );

	/** Consume energy when fired */
	void ConsumeEnergy();

	/** Handle weapon fire */
	void HandleFiring();

	/** Fire & update energy */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerHandleFiring();

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStartFire();

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStopFire();

protected:
	/** Called in network play to do the cosmetic fx for firing */
	virtual void SimulateWeaponFire();

	/** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	virtual void StopSimulatingWeaponFire();

	UFUNCTION()
	void OnRep_OwnerCharacter();

	UFUNCTION()
	void OnRep_BurstCounter();

protected:
	/** Weapon data */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	FMWeaponData WeaponData;

	/** First person weapon mesh */
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USkeletalMeshComponent* MeshFP;

	/** Third person weapon mesh */
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USkeletalMeshComponent* MeshTP;

	/** Name of bone/socket for muzzle in weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName MuzzleAttachPoint;

	/** Fire animations */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FMWeaponAnimation FireAnimation;

	/** Equip animations */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FMWeaponAnimation EquipAnimation;

	/** Is fire animation looped? */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	bool bLoopedFireAnimation;

	/** Effect for muzzle flash */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UParticleSystem* MuzzleEffect;

	/** Is muzzle effect looped? */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	bool bLoopedMuzzleEffect;

	/** Spawned component for muzzle effect */
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzleParticleSystem;

	/** Spawned component for second muzzle FX (Needed for split screen) */
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzleParticleSystemSecondary;

	/** Camera shake on firing */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<UCameraShake> FireCameraShake;

	/** Is fire animation playing? */
	bool bPlayingFireAnimation;

	/** Is equip animation playing? */
	bool bPendingEquip;

	/** Character owner */
	AMCharacter* OwnerCharacter;

	/** Current weapon state */
	EMWeaponState CurrentState;

	/** Current energy */
	float Energy;

	/** Is weapon currently equipped? */
	bool bIsEquipped;

	/** Is weapon fire active? */
	bool bWantsToFire;

	/** Is weapon is refiring? */
	bool bRefiring;

	/** Time of last successful weapon fire */
	float LastFireTime;

	/** Last time when this weapon was switched to */
	float EquipStartedTime;

	/** How much time weapon needs to be equipped */
	float EquipDuration;

	/** Burst counter, used for replicating fire events to remote clients */
	int32 BurstCounter;

	/** Handle for efficient management of OnEquipFinished timer */
	FTimerHandle TimerHandle_OnEquipFinished;

	/** Handle for efficient management of StopReload timer */
	FTimerHandle TimerHandle_StopReload;

	/** Handle for efficient management of ReloadWeapon timer */
	FTimerHandle TimerHandle_ReloadWeapon;

	/** Handle for efficient management of HandleFiring timer */
	FTimerHandle TimerHandle_HandleFiring;

	/** Play weapon animations */
	float PlayWeaponAnimation(const FMWeaponAnimation& Animation);

	/** Stop playing weapon animations */
	void StopWeaponAnimation(const FMWeaponAnimation& Animation);

	/** Get the aim of the weapon, allowing for adjustments to be made by the weapon */
	virtual FVector GetAdjustedAim() const;

	/** Get the aim of the camera */
	FVector GetCameraAim() const;

	/** Get the originating location for camera damage */
	FVector GetCameraDamageStartLocation(const FVector& AimDir) const;

	/** Get the muzzle location of the weapon */
	FVector GetMuzzleLocation() const;

	/** Get direction of weapon's muzzle */
	FVector GetMuzzleDirection() const;

	/** Find hit */
	FHitResult WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const;

	/** Determine current weapon state */
	void DetermineWeaponState();

	/** Set weapon state */
	void SetWeaponState(EMWeaponState NewState);

	/** Firing started */
	virtual void OnBurstStarted();

	/** Firing finished */
	virtual void OnBurstFinished();

	/** Attaches weapon mesh to pawn's mesh */
	void AttachMeshToPawn();

	/** Detaches weapon mesh from pawn */
	void DetachMeshFromPawn();
};
