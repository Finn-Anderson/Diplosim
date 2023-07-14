#include "Resource.h"

#include "Math/UnrealMathUtility.h"

AResource::AResource()
{
	PrimaryActorTick.bCanEverTick = true;

	Yield = FMath::RandRange(1, 5);
}

void AResource::BeginPlay()
{
	Super::BeginPlay();
	
}

void AResource::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

int32 AResource::GetYield()
{
	return Yield;
}