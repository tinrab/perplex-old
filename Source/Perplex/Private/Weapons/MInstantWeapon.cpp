// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#include "MInstantWeapon.h"


// Sets default values
AMInstantWeapon::AMInstantWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMInstantWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMInstantWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

