#include "Buildings/Misc/Broch.h"

#include "Components/DecalComponent.h"
#include "NavigationSystem.h"

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

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* navData = nav->GetDefaultNavDataInstance();

		FNavLocation location;
		nav->ProjectPointToNavigation(BuildingMesh->GetSocketLocation(name), location, FVector(400.0f, 400.0f, 20.0f));

		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(CitizenClass, location, GetActorRotation() - FRotator(0.0f, 90.0f, 0.0f));

		for (int32 i = 0; i < 2; i++)
			citizen->GivePersonalityTrait();

		citizen->BioStruct.Age = 17;
		citizen->Birthday();

		citizen->HealthComponent->AddHealth(100);
	}
}