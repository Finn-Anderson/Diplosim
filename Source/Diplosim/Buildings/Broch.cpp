#include "Buildings/Broch.h"

#include "AI/Citizen.h"
#include "HealthComponent.h"

ABroch::ABroch()
{
	NumToSpawn = 5;
}

void ABroch::FindCitizens()
{
	FVector loc = BuildingMesh->GetSocketLocation("Entrance");

	for (int i = 0; i < NumToSpawn; i++) {
		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(CitizenClass, loc, FRotator(0, 0, 0));

		citizen->BioStruct.Age = 17;
		citizen->Birthday();

		citizen->HealthComponent->Health = 100;

		Enter(citizen);
	}
}