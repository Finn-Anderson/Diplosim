#include "Vegetation.h"

void AVegetation::BeginPlay()
{
	Super::BeginPlay();

	if (MeshList.Num() > 0) {
		// No work on reload.
		int32 i = FMath::RandRange(0, (MeshList.Num() - 1));

		ResourceMesh->SetStaticMesh(MeshList[i]);

		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, ResourceMesh->GetStaticMesh()->GetName());
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