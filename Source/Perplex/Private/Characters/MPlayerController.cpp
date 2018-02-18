// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#include "MPlayerController.h"
#include "MPlayerCharacter.h"

void AMPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
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
		InputComponent->BindAction("Fire", IE_Pressed, this, &AMPlayerController::OnStartWeaponFire);
		InputComponent->BindAction("Fire", IE_Released, this, &AMPlayerController::OnStopWeaponFire);
	}
}

void AMPlayerController::BeginPlayingState()
{
	Super::BeginPlayingState();

	PlayerCharacter = Cast<AMPlayerCharacter>(GetCharacter());
}

void AMPlayerController::OnMoveHorizontal(float AxisValue)
{
	PlayerCharacter->MoveHorizontal(AxisValue);
}

void AMPlayerController::OnMoveVertical(float AxisValue)
{
	if (PlayerCharacter)
	{
		PlayerCharacter->MoveVertical(AxisValue);
	}
}

void AMPlayerController::OnLookHorizontal(float AxisValue)
{
	if (PlayerCharacter)
	{
		PlayerCharacter->AddControllerYawInput(AxisValue);
	}
}

void AMPlayerController::OnLookVertical(float AxisValue)
{
	if (PlayerCharacter)
	{
		PlayerCharacter->AddControllerPitchInput(AxisValue);
	}
}

void AMPlayerController::OnStartJump()
{
	if (PlayerCharacter)
	{
		PlayerCharacter->StartJump();
	}
}

void AMPlayerController::OnStopJump()
{
	if (PlayerCharacter)
	{
		PlayerCharacter->StopJump();
	}
}

void AMPlayerController::OnStartRun()
{
	if (PlayerCharacter)
	{
		PlayerCharacter->StartRun();
	}
}

void AMPlayerController::OnStopRun()
{
	if (PlayerCharacter)
	{
		PlayerCharacter->StopRun();
	}
}

void AMPlayerController::OnStartAim()
{
	if (PlayerCharacter)
	{
		PlayerCharacter->StartAim();
	}
}

void AMPlayerController::OnStopAim()
{
	if (PlayerCharacter)
	{
		PlayerCharacter->StopAim();
	}
}

void AMPlayerController::OnStartWeaponFire()
{
	if (PlayerCharacter)
	{
		PlayerCharacter->StartWeaponFire();
	}
}

void AMPlayerController::OnStopWeaponFire()
{
	if (PlayerCharacter)
	{
		PlayerCharacter->StopWeaponFire();
	}
}
