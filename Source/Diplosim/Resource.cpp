#include "Resource.h"

#include "Math/UnrealMathUtility.h"

#include "Grid.h"
#include "Tile.h"

AResource::AResource()
{
	PrimaryActorTick.bCanEverTick = false;

	ResourceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResourceMesh"));
	ResourceMesh->SetMobility(EComponentMobility::Static);
	ResourceMesh->bCastDynamicShadow = true;
	ResourceMesh->CastShadow = true;

	Quantity = 100000;

	MaxYield = 5;

	isRock = true;
}

int32 AResource::GetYield()
{
	Yield = GenerateYield();

	YieldStatus();

	return Yield;
}

int32 AResource::GenerateYield()
{
	int32 yield = FMath::RandRange(1, MaxYield);

	return yield;
}

void AResource::YieldStatus() 
{
	if (isRock) {
		Quantity -= Yield;

		if (Quantity <= 0) {
			FVector loc = GetActorLocation();
			Grid->GenerateTile(Grid->Hill, 0, loc.X, loc.Y);

			Destroy();
		}
	}
	else {
		SetActorHiddenInGame(true);

		FTimerHandle growTimer;
		GetWorldTimerManager().SetTimer(growTimer, this, &AResource::Grow, 300.0f, true);
	}
}

void AResource::SetQuantity(int32 Value)
{
	Quantity = Value;
}

void AResource::Grow()
{
	SetActorHiddenInGame(false);
}