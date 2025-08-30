#include "Buildings/Misc/Broch.h"

#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Resources/Mineral.h"

ABroch::ABroch()
{
	HealthComponent->MaxHealth = 300;
	HealthComponent->Health = HealthComponent->MaxHealth;

	BuildingMesh->bReceivesDecals = false;

	DecalComponent->DecalSize = FVector(2000.0f, 2000.0f, 2000.0f);
	DecalComponent->SetVisibility(true);
}

void ABroch::SpawnCitizens()
{
	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	for (int32 i = 0; i < Camera->CitizenNum; i++) {
		FNavLocation navLoc;
		nav->GetRandomReachablePointInRadius(GetActorLocation(), 400.0f, navLoc);

		FActorSpawnParameters params;
		params.bNoFail = true;

		FTransform transform;
		transform.SetLocation(navLoc.Location);
		transform.SetRotation((GetActorRotation() - FRotator(0.0f, 90.0f, 0.0f)).Quaternion());

		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(CitizenClass, FVector::Zero(), FRotator::ZeroRotator, params);
		Camera->Grid->AIVisualiser->AddInstance(citizen, Camera->Grid->AIVisualiser->HISMCitizen, transform);

		citizen->CitizenSetup(faction);

		citizen->SetSex(faction->Citizens);

		for (int32 j = 0; j < 2; j++)
			citizen->GivePersonalityTrait();

		citizen->BioStruct.Age = 17;
		citizen->Birthday();
		
		citizen->HealthComponent->MaxHealth = 100 * citizen->HealthComponent->HealthMultiplier;
		citizen->HealthComponent->AddHealth(100 * citizen->HealthComponent->HealthMultiplier);
	}
}