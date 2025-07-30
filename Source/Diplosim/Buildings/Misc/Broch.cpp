#include "Buildings/Misc/Broch.h"

#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Map/Grid.h"
#include "Map/Resources/Mineral.h"

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
	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	for (int32 i = 0; i < Camera->CitizenNum; i++) {
		FNavLocation navLoc;
		nav->GetRandomReachablePointInRadius(GetActorLocation(), 400.0f, navLoc);

		FActorSpawnParameters params;
		params.bNoFail = true;

		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(CitizenClass, navLoc.Location, GetActorRotation() - FRotator(0.0f, 90.0f, 0.0f), params);

		citizen->SetSex(Camera->CitizenManager->Citizens);

		citizen->MainIslandSetup();

		for (int32 j = 0; j < 2; j++)
			citizen->GivePersonalityTrait();

		citizen->BioStruct.Age = 17;
		citizen->Birthday();
		
		citizen->HealthComponent->MaxHealth = 100 * citizen->HealthComponent->HealthMultiplier;
		citizen->HealthComponent->AddHealth(100 * citizen->HealthComponent->HealthMultiplier);

		citizen->ApplyResearch();

		citizen->SetActorLocation(navLoc.Location + FVector(0.0f, 0.0f, citizen->Capsule->GetScaledCapsuleHalfHeight()));
	}
}