#include "Buildings/Misc/Broch.h"

#include "Components/DecalComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NavigationSystem.h"

#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BioComponent.h"
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
	FNavLocation origin;
	nav->ProjectPointToNavigation(GetActorLocation(), origin);

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	TArray<int32> edgeInstances = Camera->Grid->HISMGround->GetInstancesOverlappingSphere(GetActorLocation(), 500.0f);
	GetNavigableInstances(nav, origin, Camera->Grid->HISMGround, edgeInstances);

	TArray<int32> flatInstances = Camera->Grid->HISMFlatGround->GetInstancesOverlappingSphere(GetActorLocation(), 500.0f);
	GetNavigableInstances(nav, origin, Camera->Grid->HISMFlatGround, flatInstances);

	for (int32 i = 0; i < Camera->CitizenNum; i++) {
		int32 chosenNum = Camera->Stream.RandRange(0, edgeInstances.Num() + flatInstances.Num() - 1);

		FTransform t;
		if (chosenNum > edgeInstances.Num() - 1) {
			int32 inst = flatInstances[chosenNum - edgeInstances.Num()];
			Camera->Grid->HISMFlatGround->GetInstanceTransform(inst, t);
		}
		else {
			int32 inst = edgeInstances[chosenNum];
			Camera->Grid->HISMGround->GetInstanceTransform(inst, t);
		}

		t.SetLocation(t.GetLocation() + FVector(Camera->Stream.FRandRange(-50.0f, 50.0f), Camera->Stream.FRandRange(-50.0f, 50.0f), 100.0f));

		FNavLocation navLoc;
		nav->ProjectPointToNavigation(t.GetLocation(), navLoc);

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

void ABroch::GetNavigableInstances(UNavigationSystemV1* Nav, FNavLocation Origin, UHierarchicalInstancedStaticMeshComponent* HISM, TArray<int32>& Instances)
{
	for (int32 i = Instances.Num() - 1; i > -1; i--) {
		FTransform transform;
		Camera->Grid->HISMGround->GetInstanceTransform(Instances[i], transform);
		transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, 100.0f));

		FNavLocation navLoc;
		Nav->ProjectPointToNavigation(transform.GetLocation(), navLoc);

		double length = 10000000.0f;
		ENavigationQueryResult::Type result = Nav->GetPathLength(Origin, navLoc, length);

		if (result != ENavigationQueryResult::Success || length > 500.0f)
			Instances.RemoveAt(i);
	}
}