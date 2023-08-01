#include "Tile.h"

ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;

	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	TileMesh->bCastDynamicShadow = true;
	TileMesh->CastShadow = true;

	TileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

	RootComponent = TileMesh;
}