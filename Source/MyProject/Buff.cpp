// Fill out your copyright notice in the Description page of Project Settings.


#include "Buff.h"

// Sets default values
ABuff::ABuff()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABuff::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABuff::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

