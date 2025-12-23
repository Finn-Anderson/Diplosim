#include "Buildings/Misc/Broch.h"

#include "Components/DecalComponent.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "AI/BioComponent.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Universal/HealthComponent.h"

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
		FVector location = GetActorLocation();
		location += FRotator(0.0f, Camera->Stream.RandRange(0, 360), 0.0f).Vector() * Camera->Stream.FRandRange(120.0f, 400.0f);

		FNavLocation navLoc;
		nav->ProjectPointToNavigation(location, navLoc, FVector(300.0f, 300.0f, 9.0f));
		navLoc.Location.Z = location.Z + 4.5f;

		FActorSpawnParameters params;
		params.bNoFail = true;

		FTransform transform;
		transform.SetLocation(navLoc.Location - FVector(0.0f, 0.0f, 5.0f));
		transform.SetRotation((GetActorRotation() - FRotator(0.0f, 90.0f, 0.0f)).Quaternion());

		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(CitizenClass, FVector::Zero(), FRotator::ZeroRotator, params);
		Camera->Grid->AIVisualiser->AddInstance(citizen, Camera->Grid->AIVisualiser->HISMCitizen, transform);

		citizen->CitizenSetup(faction);

		citizen->BioComponent->SetSex(faction->Citizens);

		for (int32 j = 0; j < 2; j++)
			citizen->GivePersonalityTrait();

		citizen->BioComponent->Age = 17;
		citizen->BioComponent->Birthday();
		
		citizen->HealthComponent->MaxHealth = 100 * citizen->HealthComponent->HealthMultiplier;
		citizen->HealthComponent->AddHealth(100 * citizen->HealthComponent->HealthMultiplier);
	}
}