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

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(FirstPersonCamera);
	FirstPersonMesh->bOnlyOwnerSee = true;
	FirstPersonMesh->bOwnerNoSee = false;
	FirstPersonMesh->bCastDynamicShadow = false;
	FirstPersonMesh->bReceivesDecals = false;
	FirstPersonMesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	FirstPersonMesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	FirstPersonMesh->SetCollisionObjectType(ECC_Pawn);
	FirstPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
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

bool AMPlayerCharacter::IsFiring() const
{
	return bWantsToFire;
}

bool AMPlayerCharacter::IsAiming() const
{
	return bIsAiming;
}

void AMPlayerCharacter::StartAim()
{
	bIsAiming = true;
}

void AMPlayerCharacter::StopAim()
{
	bIsAiming = false;
}

void AMPlayerCharacter::StartRun()
{
	Super::StartRun();

	if (IsAiming())
	{
		SetAiming(false);
	}
	StopWeaponFire();
}

void AMPlayerCharacter::StartRunToggle()
{
	Super::StartRunToggle();

	if (IsAiming())
	{
		SetAiming(false);
	}
	StopWeaponFire();
}

void AMPlayerCharacter::StartWeaponFire()
{
	if (!bWantsToFire)
	{
		bWantsToFire = true;
		if (IsRunning())
		{
			Super::StopRun();
		}
		/*
		if (CurrentWeapon)
		{
			CurrentWeapon->StartFire();
		}
		*/
	}
}

void AMPlayerCharacter::StopWeaponFire()
{
	if (bWantsToFire)
	{
		bWantsToFire = false;
		/*
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
		}
		*/
	}
}

bool AMPlayerCharacter::CanFire() const
{
	return true;
}

void AMPlayerCharacter::SetAiming(bool bNewAiming)
{
	bIsAiming = bNewAiming;
}

FRotator AMPlayerCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}
