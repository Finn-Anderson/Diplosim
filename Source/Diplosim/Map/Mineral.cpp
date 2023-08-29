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
		FVector loc = (GetActorLocation() / 100) + (Grid->Size / 2);
		Grid->GenerateTile(Grid->HISMHill, 0, loc.X, loc.Y);

		Destroy();
	} else if (Quantity < MaxYield) {
		MaxYield = Quantity;
	}
}

void AMineral::SetQuantity()
{
	int32 q = FMath::RandRange(20000, 200000);

	Quantity = q;
}