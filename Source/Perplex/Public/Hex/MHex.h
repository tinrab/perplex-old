// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#pragma once

#include "Perplex.h"
#include "MHex.generated.h"

USTRUCT()
struct FMHex
{
	GENERATED_BODY()

public:
	FMHex();

	bool IsFunctional() const;

	float GetAimingSpeed() const;

	float GetRunningSpeed() const;

private:
	UPROPERTY()
	float Stability;
};
