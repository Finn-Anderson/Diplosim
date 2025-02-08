#include "Buildings/Misc/Broch.h"

#include "Components/DecalComponent.h"

#include "AI/Citizen.h"
#include "Universal/HealthComponent.h"

ABroch::ABroch()
{
	HealthComponent->MaxHealth = 300;
	HealthComponent->Health = HealthComponent->MaxHealth;

	BuildingMesh->bReceivesDecals = false;

	DecalComponent = CreateDefaultSubobject<UDecalComponent>("DecalComponent");
	DecalComponent->SetupAttachment(RootComponent);
	DecalComponent->DecalSize = FVector(2000.0f, 2000.0f, 2000.0f);
	DecalComponent->SetRelativeRotation(FRotator(-90, 0, 0));

	StorageCap = 1000000;
}

void ABroch::SpawnCitizens()
{
	TArray<FName> names = BuildingMesh->GetAllSocketNames();

	for (FName name : names) {
		if (name == FName("InfoSocket"))
			continue;

		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(CitizenClass, BuildingMesh->GetSocketLocation(name), GetActorRotation() - FRotator(0.0f, 90.0f, 0.0f));

		citizen->BioStruct.Age = 17;
		citizen->Birthday();

		citizen->HealthComponent->AddHealth(100);

		for (int32 i = 0; i < 2; i++)
			citizen->GivePersonalityTrait();
	}
}