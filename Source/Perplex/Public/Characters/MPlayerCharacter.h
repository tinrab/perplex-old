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

protected:
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UCameraComponent* FirstPersonCamera;

	virtual void Tick(float DeltaTime) override;

	virtual void BeginPlay() override;
};
