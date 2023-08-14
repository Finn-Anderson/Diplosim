#include "ExternalProduction.h"

#include "Kismet/GameplayStatics.h"

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

	AResource* resource = nullptr;

	for (int i = 0; i < foundResources.Num(); i++) {
		if (foundResources[i]->IsHidden() || (foundResources[i]->IsA<AVegetation>() && Cast<AVegetation>(foundResources[i])->bIsGettingChopped))
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