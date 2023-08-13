#include "Mineral.h"

#include "Grid.h"

AMineral::AMineral()
{
	ResourceMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	ResourceMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
}

void AMineral::YieldStatus()
{
	Quantity -= Yield;

	if (Quantity <= 0) {
		FVector loc = GetActorLocation();
		Grid->GenerateTile("Hill", 0, loc.X, loc.Y);

		Destroy();
	}
}

void AMineral::SetQuantity()
{
	int32 q = FMath::RandRange(20000, 200000);

	Quantity = q;
}