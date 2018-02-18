// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#pragma once

#include "Perplex.h"
#include "GameFramework/Character.h"
#include "MCharacter.generated.h"

class UMCharacterMovementComponent;
class AMWeapon;

/** Replicated information on a hit we've taken */
USTRUCT()
struct FMTakeHitInfo
{
	GENERATED_BODY()

	/** The amount of damage actually applied */
	UPROPERTY()
	float ActualDamage;

	/** The damage type we were hit with. */
	UPROPERTY()
	UClass* DamageTypeClass;

	/** Who hit us */
	UPROPERTY()
	TWeakObjectPtr<class AMCharacter> PawnInstigator;

	/** Who actually caused the damage */
	UPROPERTY()
	TWeakObjectPtr<class AActor> DamageCauser;

	/** Specifies which DamageEvent below describes the damage received. */
	UPROPERTY()
	int32 DamageEventClassID;

	/** Rather this was a kill */
	UPROPERTY()
	uint32 bKilled : 1;

private:
	/** A rolling counter used to ensure the struct is dirty and will replicate. */
	UPROPERTY()
	uint8 EnsureReplicationByte;

	/** Describes general damage. */
	UPROPERTY()
	FDamageEvent GeneralDamageEvent;

	/** Describes point damage, if that is what was received. */
	UPROPERTY()
	FPointDamageEvent PointDamageEvent;

	/** Describes radial damage, if that is what was received. */
	UPROPERTY()
	FRadialDamageEvent RadialDamageEvent;

public:
	FMTakeHitInfo();

	FDamageEvent& GetDamageEvent();

	void SetDamageEvent(const FDamageEvent& DamageEvent);

	void EnsureReplication();
};

UCLASS()
class PERPLEX_API AMCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostInitializeComponents() override;

	void Tick(float DeltaTime) override;

	virtual void Destroyed() override;

	virtual void PawnClientRestart() override;

	virtual void PostNetReceiveLocationAndRotation() override;

	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

public: // Weapon usage

	/** Starts weapon fire */
	void StartWeaponFire();

	/** Stops weapon fire */
	void StopWeaponFire();

	/** Check if character can fire weapon */
	bool CanFire() const;

	/** Starts aiming */
	void StartAim();

	/** Stops aiming */
	void StopAim();

public: // Movement

	void LaunchCharacterRotated(FVector LaunchVelocity, bool bHorizontalOverride, bool bVerticalOverride);

	virtual void ApplyDamageMomentum(float DamageTaken, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser) override;

	void MoveHorizontal(float Amount);

	void MoveVertical(float Amount);

	void StartJump();

	void StopJump();

	void StartRun();

	void StartRunToggle();

	void StopRun();

public: // Animations

	virtual float PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate = 1.f, FName StartSectionName = NAME_None) override;

	virtual void StopAnimMontage(class UAnimMontage* AnimMontage) override;

	void StopAllAnimMontages();

protected: // Inventory

	/** Equips weapon from inventory */
	void EquipWeapon(class AMWeapon* Weapon);

	/** Equip weapon */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerEquipWeapon(class AMWeapon* NewWeapon);

	/** Updates current weapon */
	void SetCurrentWeapon(class AMWeapon* NewWeapon, class AMWeapon* LastWeapon = nullptr);

	/** Remove all weapons from inventory and destroy them */
	void DestroyInventory();

	/** Current weapon rep handler */
	UFUNCTION()
	void OnRep_CurrentWeapon(class AMWeapon* LastWeapon);

protected: // Hex
	/** Identifies if pawn is in its dying state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex")
	bool bIsDying;

	/** Current health */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Hex")
	float Health;

	/** Take damage, handle death */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	/** Pawn suicide */
	virtual void Suicide();

	/** Kill this pawn */
	virtual void KilledBy(class APawn* EventInstigator);

	/** Returns True if the pawn can die in the current state */
	virtual bool CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const;

	virtual bool Die(float KillingDamage, struct FDamageEvent const& DamageEvent, class AController* Killer, class AActor* DamageCauser);

	/** Die when we fall out of the world */
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	/** Notification when killed, for both the server and client. */
	virtual void OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser);

protected:
	/** First person weapon mesh */
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USkeletalMeshComponent* MeshFP;

	/** Third person weapon mesh */
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USkeletalMeshComponent* MeshTP;

	/** Socket or bone name for attaching weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponAttachPoint;

	/** Currently equipped weapon */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentWeapon)
	class AMWeapon* CurrentWeapon;

	/** Default inventory list */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<TSubclassOf<AMWeapon>> DefaultInventoryClasses;

	/** Weapons in inventory */
	UPROPERTY(Transient, Replicated)
	TArray<AMWeapon*> Inventory;

	/** Replicate where this pawn was last hit and damaged */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_LastTakeHitInfo)
	struct FMTakeHitInfo LastTakeHitInfo;

	/** Animation played on termination */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* TerminationAnimation;

	/** Time at which point the last take hit info for the actor times out and won't be replicated; Used to stop join-in-progress effects all over the screen */
	float LastTakeHitTimeTimeout;

	UMCharacterMovementComponent* CharacterMovement;

	bool bWantsToRun;

	bool bWantsToRunToggled;

	bool bIsAiming;

	bool bWantsToFire;

	bool bIsTerminating;

	/** Handle mesh visibility and updates */
	void UpdateCharacterMeshes();

	/** Responsible for cleaning up bodies on clients */
	virtual void TornOff();

	/** Switch to ragdoll */
	void SetRagdollPhysics();

	/** Sets up the replication for taking a hit */
	void ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser, bool bKilled);

	/** Play hit or death on client */
	UFUNCTION()
	void OnRep_LastTakeHitInfo();

public: // Getters and setters
	UFUNCTION(BlueprintCallable, Category = "Character")
	bool IsMoving() const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	bool IsRunning() const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	bool IsFiring() const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	bool IsAiming() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AMWeapon* GetWeapon() const;

	FORCEINLINE FName GetWeaponAttachPoint() const { return WeaponAttachPoint; }

	FORCEINLINE UMCharacterMovementComponent* GetCustomCharacterMovementComponent() const { return CharacterMovement; };

	FRotator GetAimOffsets() const;

	virtual FVector GetPawnViewLocation() const override;

	bool IsEnemyFor(AController* TestController) const;

	bool IsFirstPerson() const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	float GetHealth() const;

	USkeletalMeshComponent* GetFirstPersonMesh() const;

	USkeletalMeshComponent* GetThirdPersonMesh() const;

	/** Get mesh component */
	USkeletalMeshComponent* GetCharacterMesh() const;

private:
	void SetAiming(bool bNewAiming);

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerSetAiming(bool bNewAiming);

	void SetRunning(bool bNewRunning, bool bToggle);

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerSetRunning(bool bNewRunning, bool bToggle);
};
