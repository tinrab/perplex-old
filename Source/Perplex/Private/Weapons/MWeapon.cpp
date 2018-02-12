// Copyright 2018 Tin Rabzelj. All Rights Reserved.

#include "MWeapon.h"


// Sets default values
AMWeapon::AMWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

