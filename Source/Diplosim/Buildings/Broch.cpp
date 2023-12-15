#include "Buildings/Broch.h"

#include "AI/Citizen.h"
#include "AI/HealthComponent.h"

ABroch::ABroch()
{
	NumToSpawn = 5;
}

void ABroch::FindCitizens()
{
	FVector loc = BuildingMesh->GetSocketLocation("Entrance");

	for (int i = 0; i < NumToSpawn; i++) {
		ACitizen* c = GetWorld()->SpawnActor<ACitizen>(CitizenClass, loc, FRotator(0, 0, 0));

		c->BioStruct.Age = 17;
		c->Birthday();
		c->HealthComponent->Health = c->HealthComponent->MaxHealth;

		Enter(c);
	}
}