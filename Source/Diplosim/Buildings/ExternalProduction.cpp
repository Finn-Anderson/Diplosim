#include "ExternalProduction.h"

#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "Resource.h"
#include "Map/Vegetation.h"

void AExternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (Occupied.Contains(Citizen)) {
		Store(Citizen->Carrying, Citizen);

		Citizen->Carrying = 0;
	}
}

void AExternalProduction::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);
}

void AExternalProduction::Production(ACitizen* Citizen)
{
	TArray<AActor*> foundResources;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), Resource, foundResources);

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetNavDataForProps(Citizen->GetNavAgentPropertiesRef());

	AResource* resource = nullptr;

	for (int i = 0; i < foundResources.Num(); i++) {
		FPathFindingQuery query(Citizen, *NavData, Citizen->GetActorLocation(), foundResources[i]->GetActorLocation());
		query.bAllowPartialPaths = false;

		bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

		if (foundResources[i]->IsHidden() || !path || (foundResources[i]->IsA<AVegetation>() && !Cast<AVegetation>(foundResources[i])->IsChoppable()))
			continue;

		if (resource == nullptr) {
			resource = Cast<AResource>(foundResources[i]);
		}
		else {
			float dR = (resource->GetActorLocation() - GetActorLocation()).Length();

			float dF = (foundResources[i]->GetActorLocation() - GetActorLocation()).Length();

			if (dR > dF) {
				resource = Cast<AResource>(foundResources[i]);
			}
		}
	}

	if (resource->IsA<AVegetation>()) {
		Cast<AVegetation>(resource)->bIsGettingChopped = true;
	}

	Citizen->MoveTo(resource);
}