// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#pragma once

#include "Perplex.h"
#include "GameFramework/Character.h"
#include "MCharacter.generated.h"

class UMCharacterMovementComponent;

UCLASS()
class PERPLEX_API AMCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void LaunchCharacterRotated(FVector LaunchVelocity, bool bHorizontalOverride, bool bVerticalOverride);

	FORCEINLINE UMCharacterMovementComponent* GetCustomCharacterMovementComponent() const { return CharacterMovement; };

	void Tick(float DeltaTime) override;

	virtual void ApplyDamageMomentum(float DamageTaken, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser) override;

	virtual void PostNetReceiveLocationAndRotation() override;

	virtual void MoveHorizontal(float Amount);

	virtual void MoveVertical(float Amount);

	virtual void StartJump();

	virtual void StopJump();

	virtual void StartRun();

	virtual void StartRunToggle();

	virtual void StopRun();

	void SetRunning(bool bNewRunning, bool bToggle);

	bool IsMoving();

	bool IsRunning() const;

	virtual FVector GetPawnViewLocation() const override;

	bool IsEnemyFor(AController* TestController) const;

protected:
	UMCharacterMovementComponent* CharacterMovement;

	bool bWantsToRun;

	bool bWantsToRunToggled;
};
