// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MPlayerController.generated.h"

class AMCharacter;

UCLASS()
class PERPLEX_API AMPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void SetupInputComponent() override;

	UFUNCTION()
	void OnMoveHorizontal(float AxisValue);

	UFUNCTION()
	void OnMoveVertical(float AxisValue);

	UFUNCTION()
	void OnLookHorizontal(float AxisValue);

	UFUNCTION()
	void OnLookVertical(float AxisValue);

	UFUNCTION()
	void OnStartJump();

	UFUNCTION()
	void OnStopJump();

	UFUNCTION()
	void OnStartRun();

	UFUNCTION()
	void OnStopRun();

	UFUNCTION()
	void OnStartAim();

	UFUNCTION()
	void OnStopAim();

private:
	AMCharacter* PlayerCharacter;
};
