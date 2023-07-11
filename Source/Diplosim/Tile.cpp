#include "Tile.h"

#include "Math/UnrealMathUtility.h"
#include "Components/InstancedStaticMeshComponent.h"

ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;

	ISMComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ISMC"));
	SetRootComponent(ISMComponent);

	Fertility = 0;
	Type = Water;
}

void ATile::BeginPlay()
{
	Super::BeginPlay();

	ISMComponent->AddInstance(FTransform::Identity);
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