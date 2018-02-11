// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "MWeaponDamageType.generated.h"

UCLASS()
class PERPLEX_API UMWeaponDamageType : public UDamageType
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UForceFeedbackEffect *HitForceFeedback;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UForceFeedbackEffect* KilledForceFeedback;
};
