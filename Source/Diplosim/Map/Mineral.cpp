#include "Mineral.h"

#include "Components/BoxComponent.h"

#include "Grid.h"

AMineral::AMineral()
{
	ResourceMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	ResourceMesh->SetCanEverAffectNavigation(true);
	ResourceMesh->bFillCollisionUnderneathForNavmesh = true;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	BoxCollision->SetupAttachment(ResourceMesh);
}

void AMineral::BeginPlay()
{
	Super::BeginPlay();

	FVector size = ResourceMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2;
	BoxCollision->SetBoxExtent(size + 10.0f);

	int32 q = FMath::RandRange(1000, 10000); 
	SetQuantity(q);
}

void AMineral::YieldStatus()
{
	Quantity -= Yield;

	if (Quantity <= 0) {
		Destroy();
	} else if (Quantity < MaxYield) {
		MaxYield = Quantity;
	}
}