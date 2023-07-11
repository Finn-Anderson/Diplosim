#include "Tile.h"

#include "Math/UnrealMathUtility.h"
#include "Components/InstancedStaticMeshComponent.h"

ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;

	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	TileMesh->bCastDynamicShadow = true;
	TileMesh->CastShadow = true;

	RootComponent = TileMesh;

	Fertility = 0;
	Type = Water;
}

void ATile::BeginPlay()
{
	Super::BeginPlay();
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