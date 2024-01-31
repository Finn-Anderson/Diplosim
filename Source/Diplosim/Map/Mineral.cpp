#include "Mineral.h"

#include "Components/BoxComponent.h"

#include "Grid.h"
#include "InteractableInterface.h"

AMineral::AMineral()
{
	ResourceMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	ResourceMesh->SetCanEverAffectNavigation(true);
	ResourceMesh->bFillCollisionUnderneathForNavmesh = true;

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));
}

void AMineral::BeginPlay()
{
	Super::BeginPlay();

	int32 q = FMath::RandRange(1000, 10000); 
	SetQuantity(q);
}

void AMineral::YieldStatus()
{
	Quantity -= Yield;

	InteractableComponent->SetQuantity();
	InteractableComponent->ExecuteEditEvent("Quantity");

	if (Quantity <= 0) {
		Destroy();
	} else if (Quantity < MaxYield) {
		MaxYield = Quantity;
	}
}