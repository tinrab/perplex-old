// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#include "MPlayerController.h"
#include "GameFramework/Character.h"
#include "MCharacter.h"

void AMPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent != nullptr)
	{
		// Setup axes
		InputComponent->BindAxis("MoveHorizontal", this, &AMPlayerController::OnMoveHorizontal);
		InputComponent->BindAxis("MoveVertical", this, &AMPlayerController::OnMoveVertical);
		InputComponent->BindAxis("LookHorizontal", this, &AMPlayerController::OnLookHorizontal);
		InputComponent->BindAxis("LookVertical", this, &AMPlayerController::OnLookVertical);

		// Setup actions
		InputComponent->BindAction("Jump", IE_Pressed, this, &AMPlayerController::OnStartJump);
		InputComponent->BindAction("Jump", IE_Released, this, &AMPlayerController::OnStopJump);
		InputComponent->BindAction("Run", IE_Pressed, this, &AMPlayerController::OnStartRun);
		InputComponent->BindAction("Run", IE_Released, this, &AMPlayerController::OnStopRun);
		InputComponent->BindAction("Aim", IE_Pressed, this, &AMPlayerController::OnStartAim);
		InputComponent->BindAction("Aim", IE_Released, this, &AMPlayerController::OnStopAim);

		PlayerCharacter = Cast<AMCharacter>(GetCharacter());
	}
}

void AMPlayerController::OnMoveHorizontal(float AxisValue)
{
	PlayerCharacter->MoveHorizontal(AxisValue);
}

void AMPlayerController::OnMoveVertical(float AxisValue)
{
	PlayerCharacter->MoveVertical(AxisValue);
}

void AMPlayerController::OnLookHorizontal(float AxisValue)
{
	PlayerCharacter->AddControllerYawInput(AxisValue);
}

void AMPlayerController::OnLookVertical(float AxisValue)
{
	PlayerCharacter->AddControllerPitchInput(AxisValue);
}

void AMPlayerController::OnStartJump()
{
	PlayerCharacter->StartJump();
}

void AMPlayerController::OnStopJump()
{
	PlayerCharacter->StopJump();
}

void AMPlayerController::OnStartRun()
{
	PlayerCharacter->StartRun();
}

void AMPlayerController::OnStopRun()
{
	PlayerCharacter->StopRun();
}

void AMPlayerController::OnStartAim()
{
	PlayerCharacter->StartAim();
}

void AMPlayerController::OnStopAim()
{
	PlayerCharacter->StopAim();
}
