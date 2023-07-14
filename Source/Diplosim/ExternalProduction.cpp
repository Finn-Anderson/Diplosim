#include "ExternalProduction.h"

#include "Kismet/GameplayStatics.h"

#include "Citizen.h"
#include "AIController.h"
#include "Resource.h"

void AExternalProduction::Production(ACitizen* Citizen)
{
	TArray<AActor*> foundResources;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActorToGetResource, foundResources);

	AResource* resource = Cast<AResource>(foundResources[0]);

	for (int i = 1; i < foundResources.Num(); i++) {
		float dR = (resource->GetActorLocation() - GetActorLocation()).Length();

		float dF = (foundResources[i]->GetActorLocation() - GetActorLocation()).Length();

		if (dR > dF) {
			resource = Cast<AResource>(foundResources[i]);
		}
	}

	Citizen->MoveTo(resource);

	Citizen->Goal = resource;
}

void AExternalProduction::StoreResource(AResource* Resource)
{
	Storage += Resource->GetYield();
}