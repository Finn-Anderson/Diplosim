#include "Buildings/Misc/Broch.h"

#include "Components/DecalComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
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

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	TArray<FHitResult> hits;
	GetWorld()->SweepMultiByChannel(hits, GetActorLocation(), GetActorLocation(), FQuat::Identity, ECC_GameTraceChannel1, FCollisionShape::MakeSphere(500.0f));
	GetNavigableInstances(nav, hits);

	for (int32 i = 0; i < Camera->CitizenNum; i++) {
		int32 chosenNum = Camera->Stream.RandRange(0, hits.Num() - 1);

		FTransform t;
		Cast<UHierarchicalInstancedStaticMeshComponent>(hits[chosenNum].GetComponent())->GetInstanceTransform(hits[chosenNum].Item, t);
		t.SetLocation(t.GetLocation() + FVector(Camera->Stream.FRandRange(-40.0f, 40.0f), Camera->Stream.FRandRange(-40.0f, 40.0f), 100.0f));

		FNavLocation navLoc;
		nav->ProjectPointToNavigation(t.GetLocation(), navLoc, FVector(10.0f, 10.0f, 40.0f));

		FActorSpawnParameters params;
		params.bNoFail = true;

		FTransform transform;
		transform.SetLocation(navLoc.Location - FVector(0.0f, 0.0f, 2.0f));
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

void ABroch::GetNavigableInstances(UNavigationSystemV1* Nav, TArray<FHitResult>& Hits)
{
	const ANavigationData* navData = Nav->GetDefaultNavDataInstance();

	FNavLocation origin;
	Nav->ProjectPointToNavigation(GetActorLocation(), origin, FVector(300.0f, 300.0f, 40.0f));

	for (int32 i = Hits.Num() - 1; i > -1; i--) {
		if (Hits[i].GetComponent() != Camera->Grid->HISMGround && Hits[i].GetComponent() != Camera->Grid->HISMFlatGround) {
			Hits.RemoveAt(i);

			continue;
		}

		FTransform transform;
		Cast<UHierarchicalInstancedStaticMeshComponent>(Hits[i].GetComponent())->GetInstanceTransform(Hits[i].Item, transform);
		transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, 100.0f));

		FNavLocation navLoc;
		Nav->ProjectPointToNavigation(transform.GetLocation(), navLoc, FVector(10.0f, 10.0f, 40.0f));

		FPathFindingQuery query(this, *navData, origin.Location, navLoc.Location);

		bool path = Nav->TestPathSync(query, EPathFindingMode::Hierarchical);
		if (!path) {
			Hits.RemoveAt(i);

			continue;
		}

		double length = 10000000.0f;
		ENavigationQueryResult::Type result = Nav->GetPathLength(origin, navLoc, length);

		if (result != ENavigationQueryResult::Success || length > 700.0f)
			Hits.RemoveAt(i);
	}
}