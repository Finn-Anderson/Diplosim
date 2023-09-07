#include "Resource.h"

AResource::AResource()
{
	PrimaryActorTick.bCanEverTick = false;

	ResourceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResourceMesh"));
	ResourceMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	ResourceMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);
	ResourceMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	ResourceMesh->SetMobility(EComponentMobility::Static);
	ResourceMesh->SetCanEverAffectNavigation(false);
	ResourceMesh->bCastDynamicShadow = true;
	ResourceMesh->CastShadow = true;

	MinYield = 1;
	MaxYield = 5;

	Quantity = MaxYield;
}

int32 AResource::GetYield()
{
	Yield = GenerateYield();

	YieldStatus();

	return Yield;
}

int32 AResource::GenerateYield()
{
	int32 yield = FMath::RandRange(MinYield, MaxYield);

	return yield;
}

void AResource::SetQuantity(int32 Value)
{
	Quantity = Value;
}

void AResource::YieldStatus() 
{

}