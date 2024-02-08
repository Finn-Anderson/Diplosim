#include "Buildings/Broch.h"

#include "Components/DecalComponent.h"

#include "AI/Citizen.h"
#include "HealthComponent.h"

ABroch::ABroch()
{
	NumToSpawn = 5;

	BuildingMesh->bReceivesDecals = false;

	DecalComponent = CreateDefaultSubobject<UDecalComponent>("DecalComponent");
	DecalComponent->SetupAttachment(RootComponent);
	DecalComponent->DecalSize = FVector(2000.0f, 2000.0f, 2000.0f);
	DecalComponent->SetRelativeRotation(FRotator(-90, 0, 0));
}

void ABroch::FindCitizens()
{
	TArray<FName> names = BuildingMesh->GetAllSocketNames();

	for (FName name : names) {
		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(CitizenClass, BuildingMesh->GetSocketLocation(name), FRotator(0, 0, 0));

		citizen->BioStruct.Age = 17;
		citizen->Birthday();

		citizen->HealthComponent->AddHealth(100);

		Enter(citizen);
	}
}