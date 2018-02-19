// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#pragma once

#include "Perplex.h"
#include "Weapons/MWeapon.h"
#include "MInstantWeapon.generated.h"

class AMImpactEffect;

USTRUCT()
struct FMInstantHitInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Origin;

	UPROPERTY()
	float ReticleSpread;

	UPROPERTY()
	int32 RandomSeed;
};

USTRUCT()
struct FMInstantWeaponData
{
	GENERATED_BODY()

	/** Base weapon spread (degrees) */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	float WeaponSpread;

	/** Aim spread modifier */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	float AimSpreadModifier;

	/** Continuous firing: spread increment */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	float FiringSpreadIncrement;

	/** Continuous firing: max increment */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	float FiringSpreadMax;

	/** Weapon range */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	float WeaponRange;

	/** Damage amount */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	int32 HitDamage;

	/** Type of damage */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TSubclassOf<UDamageType> DamageType;

	/** Hit verification: scale for bounding box of hit actor */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	float ClientSideHitLeeway;

	/** Hit verification: threshold for dot product between view direction and hit direction */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	float AllowedViewDotHitDir;

	FMInstantWeaponData();
};

UCLASS()
class PERPLEX_API AMInstantWeapon : public AMWeapon
{
	GENERATED_BODY()

public:
	AMInstantWeapon();

protected:
	/** Weapon config */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	FMInstantWeaponData InstantData;

	/** Impact effects */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<AMImpactEffect> ImpactTemplate;

	/** Smoke trail */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UParticleSystem* TrailFX;

	/** Param name for beam target in smoke trail */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	FName TrailTargetParam;

	/** Instant hit notify for replication */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_HitNotify)
	FMInstantHitInfo HitNotify;

	/** Current spread from continuous firing */
	float CurrentFiringSpread;

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** Server notified of hit from client to verify */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerNotifyHit(const FHitResult& Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);
	bool ServerNotifyHit_Validate(const FHitResult& Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);
	void ServerNotifyHit_Implementation(const FHitResult& Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);

	/** Server notified of miss to show trail FX */
	UFUNCTION(Unreliable, Server, WithValidation)
	void ServerNotifyMiss(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);
	bool ServerNotifyMiss_Validate(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);
	void ServerNotifyMiss_Implementation(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);

	/** Process the instant hit and notify the server if necessary */
	void ProcessInstantHit(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread);

	/** Continue processing the instant hit, as if it has been confirmed by the server */
	void ProcessInstantHit_Confirmed(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread);

	/** Check if weapon should deal damage to actor */
	bool ShouldDealDamage(AActor* TestActor) const;

	/** Handle damage */
	void DealDamage(const FHitResult& Impact, const FVector& ShootDir);

	/** Weapon specific fire implementation */
	virtual void FireWeapon() override;

	/** Update spread on firing */
	virtual void OnBurstFinished() override;

	//////////////////////////////////////////////////////////////////////////
	// Effects replication

	UFUNCTION()
	void OnRep_HitNotify();

	/** Called in network play to do the cosmetic fx  */
	void SimulateInstantHit(const FVector& Origin, int32 RandomSeed, float ReticleSpread);

	/** Spawn effects for impact */
	void SpawnImpactEffects(const FHitResult& Impact);

	/** Spawn trail effect */
	void SpawnTrailEffect(const FVector& EndPoint);

private:
	/** Get current spread */
	float GetCurrentSpread() const;
};
