// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#include "MHex.h"

FMHex::FMHex()
	: Stability(10.0f)
{
}

bool FMHex::IsFunctional() const
{
	return Stability >= 0.0f;
}

float FMHex::GetAimingSpeed() const
{
	return 0.5f;
}

float FMHex::GetRunningSpeed() const
{
	return 3.0f;
}
