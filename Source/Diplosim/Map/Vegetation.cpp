#include "Vegetation.h"

void AVegetation::BeginPlay()
{
	Super::BeginPlay();

	if (MeshList.Num() > 0) {
		int32 i = FMath::RandRange(0, (MeshList.Num() - 1));

		ResourceMesh->SetMobility(EComponentMobility::Movable);

		ResourceMesh->SetStaticMesh(MeshList[i]);

		ResourceMesh->SetMobility(EComponentMobility::Static);
	}
}

void AVegetation::YieldStatus()
{
	SetActorHiddenInGame(true);

	FTimerHandle growTimer;
	GetWorldTimerManager().SetTimer(growTimer, this, &AVegetation::Grow, 300.0f, false);
}

void AVegetation::Grow()
{
	SetActorHiddenInGame(false);
}