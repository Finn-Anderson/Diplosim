#include "Buildings/Broch.h"

#include "AI/Citizen.h"

ABroch::ABroch()
{
	NumToSpawn = 5;
}

void ABroch::FindCitizens()
{
	FVector loc = BuildingMesh->GetSocketLocation("Entrance");

	for (int i = 0; i < NumToSpawn; i++) {
		ACitizen* c = GetWorld()->SpawnActor<ACitizen>(CitizenClass, loc + FVector(0.0f, (50.0f * i) - (50.0f * (NumToSpawn - 1) / 2), 15.0f), FRotator(0, 0, 0));

		c->Age = 17;
		c->Birthday();
	}
}