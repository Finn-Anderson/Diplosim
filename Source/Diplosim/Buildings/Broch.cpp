#include "Buildings/Broch.h"

#include "AI/Citizen.h"

ABroch::ABroch()
{
	NumToSpawn = 5;
}

void ABroch::FindCitizens()
{
	FVector loc = BuildingMesh->GetSocketLocation("SpawnRef");

	for (int i = 0; i < NumToSpawn; i++) {
		ACitizen* c = GetWorld()->SpawnActor<ACitizen>(CitizenClass, loc + FVector(0.0f, 50.0f * i, 0.0f), FRotator(0, 0, 0));

		c->Age = 17;
		c->Birthday();
	}
}