#include "Vegetation.h"

AVegetation::AVegetation()
{

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