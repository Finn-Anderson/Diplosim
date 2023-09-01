#include "Mineral.h"

#include "Components/BoxComponent.h"

#include "Grid.h"

AMineral::AMineral()
{
	ResourceMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	ResourceMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

	BoxCollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollisionComp->SetupAttachment(ResourceMesh);
	BoxCollisionComp->bDynamicObstacle = true;
}

void AMineral::BeginPlay()
{
	Super::BeginPlay();

	FVector size = ResourceMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2.5;
	BoxCollisionComp->SetBoxExtent(size);
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
	int32 q = FMath::RandRange(1000, 10000);

	Quantity = q;
}