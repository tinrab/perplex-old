// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#include "MPlayerCharacter.h"
#include "MCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Camera/CameraComponent.h"

AMPlayerCharacter::AMPlayerCharacter()
{
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, BaseEyeHeight));
}

void AMPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AMPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/*
	FRotator Rotation = GetActorRotation();
	const FRotator ControlRotation = GetControlRotation();
	Rotation.SetComponentForAxis(EAxis::Z, ControlRotation.GetComponentForAxis(EAxis::Z));
	FirstPersonCameraComponent->SetWorldRotation(Rotation);
	*/

	FRotator Rotation = GetControlRotation();
	Rotation.SetComponentForAxis(EAxis::X, 0.0f);
	FirstPersonCamera->SetRelativeRotation(Rotation);
}
