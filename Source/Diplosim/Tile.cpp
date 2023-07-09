#include "Tile.h"

#include "Math/UnrealMathUtility.h"

ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = true;

	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AIMesh"));
	TileMesh->SetupAttachment(RootComponent);
	TileMesh->CastShadow = true;

	Fertility = 0;
	Type = Water;
}

void ATile::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATile::SetFertility(int32 Mean) 
{
	int32 value = FMath::RandRange(-1, 1);

	Fertility = Mean + value;
}

int32 ATile::GetFertility() 
{
	return Fertility;
}

TEnumAsByte<EType> ATile::GetType()
{
	return Type;
}