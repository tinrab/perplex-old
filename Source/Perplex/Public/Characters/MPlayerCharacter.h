// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MCharacter.h"
#include "MPlayerCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;

UCLASS()
class PERPLEX_API AMPlayerCharacter : public AMCharacter
{
	GENERATED_BODY()

public:
	AMPlayerCharacter();

	UFUNCTION(BlueprintCallable, Category = "Player Character")
	bool IsFiring() const;

	UFUNCTION(BlueprintCallable, Category = "Player Character")
	bool IsAiming() const;

	void StartAim();

	void StopAim();

	virtual void StartRun() override;

	virtual void StartRunToggle() override;

	void StartWeaponFire();

	void StopWeaponFire();

	bool CanFire() const;

	FRotator GetAimOffsets() const;

protected:
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UCameraComponent* FirstPersonCamera;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USkeletalMeshComponent* FirstPersonMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponAttachPoint;

	bool bIsAiming;

	bool bWantsToFire;

	virtual void Tick(float DeltaTime) override;

	virtual void BeginPlay() override;

private:
	void SetAiming(bool bNewAiming);
};
