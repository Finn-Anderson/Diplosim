#include "Buildings/Misc/Special/PowerPlant.h"

#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

#include "Player/Camera.h"

APowerPlant::APowerPlant()
{
	bNoTimer = true;
}

void APowerPlant::OnBuilt()
{
	TArray<FVector> socketLocations;

	for (FName socketName : BuildingMesh->GetAllSocketNames()) {
		if (socketName == "Entrance")
			continue;

		socketLocations.Add(BuildingMesh->GetSocketLocation(socketName) + GetActorLocation());
	}

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(ParticleComponent, TEXT("SpawnLocations"), socketLocations);

	Super::OnBuilt();

	Camera->UnlockAchievement("FeatheredFuel");
}