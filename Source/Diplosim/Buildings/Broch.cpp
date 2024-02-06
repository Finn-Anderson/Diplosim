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
	double yPos = -BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Y / 2;

	FVector loc = GetActorLocation() + FVector(0.0f, yPos, 0.0f);

	for (int i = 0; i < NumToSpawn; i++) {
		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(CitizenClass, loc, FRotator(0, 0, 0));

		citizen->BioStruct.Age = 17;
		citizen->Birthday();

		citizen->HealthComponent->AddHealth(100);

		Enter(citizen);
	}
}