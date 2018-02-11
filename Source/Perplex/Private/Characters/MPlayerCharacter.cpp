// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#include "MPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Camera/CameraComponent.h"

AMPlayerCharacter::AMPlayerCharacter()
{
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, BaseEyeHeight));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->bCastDynamicShadow = false;
	FirstPersonMesh->CastShadow = false;
	FirstPersonMesh->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	FirstPersonMesh->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);
	FirstPersonMesh->SetupAttachment(FirstPersonCamera);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));
	WeaponMesh->SetOnlyOwnerSee(true);
	WeaponMesh->bCastDynamicShadow = false;
	WeaponMesh->CastShadow = false;
	WeaponMesh->SetupAttachment(GetCapsuleComponent());
}

void AMPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	WeaponMesh->AttachToComponent(FirstPersonMesh, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));
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
