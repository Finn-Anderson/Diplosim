#include "ExternalProduction.h"

#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "Resource.h"
#include "Map/Vegetation.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"

void AExternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (Occupied.Contains(Citizen)) {
		Store(Citizen->Carrying.Amount, Citizen);
	}
}

void AExternalProduction::Production(ACitizen* Citizen)
{
	if (AtWork.Contains(Citizen)) {
		if (Resource == nullptr || !Resource->IsValidLowLevelFast() || Resource->Quantity <= 0) {
			TArray<AActor*> foundResources;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), Camera->ResourceManagerComponent->GetResource(this), foundResources);

			Resource = nullptr;

			for (int32 i = 0; i < foundResources.Num(); i++) {
				AResource* r = Cast<AResource>(foundResources[i]);

				FVector loc = Citizen->CanMoveTo(r);

				if (r->IsHidden() || loc.IsZero() || r->Quantity <= 0)
					continue;

				Resource = Cast<AResource>(Citizen->GetClosestActor(Citizen, Resource, r));
			}
		}

		if (Resource != nullptr) {
			if (Resource->IsA<AVegetation>()) {
				Resource->Quantity = 0;
			}

			Citizen->MoveTo(Resource);
		}
		else {
			GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AExternalProduction::Production, Citizen), 30.0f, false);
		}
	}
}