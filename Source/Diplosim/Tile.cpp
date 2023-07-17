#include "Tile.h"

#include "Math/UnrealMathUtility.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "Resource.h"

ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;

	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	TileMesh->bCastDynamicShadow = true;
	TileMesh->CastShadow = true;

	TileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

	RootComponent = TileMesh;

	Fertility = 0;
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

void ATile::GenerateTree()
{
	FVector origin;
	FVector boxExtent;
	GetActorBounds(false, origin, boxExtent);

	int32 xRand;
	int32 yRand;

	bool passed = false;

	if (Trees.Num() > 0) {
		while (!passed) {
			xRand = FMath::RandRange(-45, 45);
			yRand = FMath::RandRange(-45, 45);


			for (int j = 0; j < Trees.Num(); j++) {

				int32 xT = Trees[j]->GetActorLocation().X;
				int32 yT = Trees[j]->GetActorLocation().Y;


				if (!((xRand < (xT + 10) && xRand >(xT - 10)) || (yRand < (yT + 10) && yRand >(yT - 10)))) {
					passed = true;
				}
			}
		}
	}
	else {
		xRand = FMath::RandRange(-45, 45);
		yRand = FMath::RandRange(-45, 45);
	}

	FVector location = FVector(GetActorLocation().X + xRand, GetActorLocation().Y + yRand, boxExtent.Z + origin.Z);

	AResource* tree = GetWorld()->SpawnActor<AResource>(Tree, location, GetActorRotation());

	Trees.Add(tree);
}