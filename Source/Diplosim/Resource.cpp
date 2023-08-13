#include "Resource.h"

AResource::AResource()
{
	PrimaryActorTick.bCanEverTick = false;

	ResourceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResourceMesh"));
	ResourceMesh->SetMobility(EComponentMobility::Static);
	ResourceMesh->SetCanEverAffectNavigation(false);
	ResourceMesh->bCastDynamicShadow = true;
	ResourceMesh->CastShadow = true;

	MaxYield = 5;
}

int32 AResource::GetYield()
{
	Yield = GenerateYield();

	YieldStatus();

	return Yield;
}

int32 AResource::GenerateYield()
{
	int32 yield = FMath::RandRange(1, MaxYield);

	return yield;
}

void AResource::YieldStatus() 
{

}