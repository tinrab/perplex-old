// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Canvas.h"
#include "MWeapon.generated.h"

class UAnimMontage;
class AMCharacter;
class UAudioComponent;
class UParticleSystemComponent;
class UCameraShake;
class USoundCue;

UENUM()
enum class EWeaponState : uint8
{
	Idle,
	Firing,
	Equipping
};

USTRUCT()
struct FWeaponData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float MaxEnergy;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float Consumption;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float FireRate;
};

USTRUCT()
struct FWeaponAnim
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* FirstPersonPawn;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ThirdPersonPawn;
};

UCLASS(Abstract)
class PERPLEX_API AMWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AMWeapon();

	virtual void PostInitializeComponents() override;

	virtual void Destroyed() override;

	/** Get current weapon state */
	FORCEINLINE EWeaponState GetCurrentState() const { return CurrentState; };

	/** Get current energy amount */
	FORCEINLINE float GetCurrentEnergy() const { return CurrentEnergy; };

	/** Get max energy amount */
	FORCEINLINE float GetMaxEnergy() const { return WeaponConfig.MaxEnergy; };

	/** [server] Add energy */
	void AddEnergy(float Amount);

	/** Consume energy */
	void ConsumeEnergy(float Amount);

	/** Get first person mesh */
	FORCEINLINE USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; };

	/** Get third person mesh */
	FORCEINLINE USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; };

	/** Get weapon mesh (needs pawn owner to determine variant) */
	USkeletalMeshComponent* GetWeaponMesh() const;

	/** Get owning pawn */
	AMCharacter* GetPawnOwner() const;

	/** Set owning pawn */
	void SetOwningPawn(AMCharacter* Pawn);

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** Weapon is being equipped by owner pawn */
	virtual void OnEquip(const AMWeapon* LastWeapon);

	/** Weapon is now equipped by owner pawn */
	virtual void OnEquipFinished();

	/** Weapon is holstered by owner pawn */
	virtual void OnUnEquip();

	/** Check if it's currently equipped */
	bool IsEquipped() const;

	/** Check if mesh is already attached */
	bool IsAttachedToPawn() const;


	//////////////////////////////////////////////////////////////////////////
	// Input

	/** [local + server] Start weapon fire */
	virtual void StartFire();

	/** [local + server] Stop weapon fire */
	virtual void StopFire();

	/** Check if weapon can fire */
	bool CanFire() const;

protected:
	/** Crosshair icon */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FCanvasIcon Crosshair;

	/** Pawn owner */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_MyPawn)
	AMCharacter* MyPawn;

	/** Weapon data */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	FWeaponData WeaponConfig;

	/** First person weapon mesh */
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USkeletalMeshComponent* FirstPersonMesh;

	/** Third person weapon mesh */
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USkeletalMeshComponent* ThirdPersonMesh;

	/** Firing audio component */
	UPROPERTY(Transient)
	UAudioComponent* FireAudio;

	/** Name of socket for muzzle in weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	FName MuzzleAttachPoint;

	/** Effect for muzzle flash */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UParticleSystem* MuzzleEffect;

	/** Component for muzzle flash effect */
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzleEffectComponent;

	/** Camera shake on firing */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<UCameraShake> FireCameraShake;

	/** Single fire sound (bLoopedFireSound not set) */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* FireSound;

	/** Looped fire sound (bLoopedFireSound set) */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* FireLoopSound;

	/** Finished burst sound (bLoopedFireSound set) */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* FireFinishSound;

	/** Out of energy sound */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* OutOfAmmoSound;

	/** Equip sound */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* EquipSound;

	/** Equip animations */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FWeaponAnim EquipAnim;

	/** Fire animations */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FWeaponAnim FireAnim;

	/** Is muzzle FX looped? */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	uint32 bLoopedMuzzleFX : 1;

	/** Is fire sound looped? */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	uint32 bLoopedFireSound : 1;

	/** Is fire animation looped? */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	uint32 bLoopedFireAnim : 1;

	/** Is fire animation playing? */
	uint32 bPlayingFireAnim : 1;

	/** Is weapon currently equipped? */
	uint32 bIsEquipped : 1;

	/** Is weapon fire active? */
	uint32 bWantsToFire : 1;

	/** Is equip animation playing? */
	uint32 bPendingEquip : 1;

	/** Weapon is refiring */
	uint32 bRefiring;

	/** Current weapon state */
	EWeaponState CurrentState;

	/** Time of last successful weapon fire */
	float LastFireTime;

	/** Current total energy */
	UPROPERTY(Transient, Replicated)
	float CurrentEnergy;

	/** Burst counter, used for replicating fire events to remote clients */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_BurstCounter)
	int32 BurstCounter;

	/** Handle for efficient management of OnEquipFinished timer */
	FTimerHandle TimerHandle_OnEquipFinished;

	/** Handle for efficient management of HandleFiring timer */
	FTimerHandle TimerHandle_HandleFiring;


	//////////////////////////////////////////////////////////////////////////
	// Input - server side

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartFire();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopFire();


	//////////////////////////////////////////////////////////////////////////
	// Replication & effects

	UFUNCTION()
	void OnRep_MyPawn();

	UFUNCTION()
	void OnRep_BurstCounter();

	UFUNCTION()
	void OnRep_Reload();

	/** Called in network play to do the cosmetic effect for firing */
	virtual void SimulateWeaponFire();

	/** Called in network play to stop cosmetic effect (e.g. for a looping shot). */
	virtual void StopSimulatingWeaponFire();


	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] Weapon specific fire implementation */
	virtual void FireWeapon() PURE_VIRTUAL(AMWeapon::FireWeapon, );

	/** [server] Fire & update energy */
	UFUNCTION(reliable, server, WithValidation)
	void ServerHandleFiring();

	/** [local + server] Handle weapon fire */
	void HandleFiring();

	/** [local + server] Firing started */
	virtual void OnBurstStarted();

	/** [local + server] Firing finished */
	virtual void OnBurstFinished();

	/** Update weapon state */
	void SetWeaponState(EWeaponState NewState);

	/** Determine current weapon state */
	void DetermineWeaponState();


	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** attaches weapon mesh to pawn's mesh */
	void AttachMeshToPawn();

	/** detaches weapon mesh from pawn */
	void DetachMeshFromPawn();


	//////////////////////////////////////////////////////////////////////////
	// Weapon usage helpers

	/** Play weapon sounds */
	UAudioComponent* PlayWeaponSound(USoundCue* Sound);

	/** Play weapon animations */
	float PlayWeaponAnimation(const FWeaponAnim& Animation);

	/** Stop playing weapon animations */
	void StopWeaponAnimation(const FWeaponAnim& Animation);

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
};
