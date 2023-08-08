#include "ExternalProduction.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"
#include "AIController.h"
#include "Resource.h"

void AExternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (Occupied.Contains(Citizen) && Citizen->Carrying > 0) {
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

	AResource* resource = Cast<AResource>(foundResources[0]);

	for (int i = 1; i < foundResources.Num(); i++) {
		if (resource->IsHidden())
			continue;

		float dR = (resource->GetActorLocation() - GetActorLocation()).Length();

		float dF = (foundResources[i]->GetActorLocation() - GetActorLocation()).Length();

		if (dR > dF) {
			resource = Cast<AResource>(foundResources[i]);
		}
	}

	Citizen->MoveTo(resource);

	Citizen->Goal = resource;
}