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

			UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
			const ANavigationData* NavData = nav->GetNavDataForProps(Citizen->GetNavAgentPropertiesRef());

			Resource = nullptr;

			for (int32 i = 0; i < foundResources.Num(); i++) {
				FPathFindingQuery query(Citizen, *NavData, Citizen->GetActorLocation(), foundResources[i]->GetActorLocation());
				query.bAllowPartialPaths = false;

				bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

				AResource* r = Cast<AResource>(foundResources[i]);

				if (r->IsHidden() || !path || r->Quantity <= 0)
					continue;

				if (Resource == nullptr) {
					Resource = r;
				}
				else {
					float dR = FVector::Dist(Resource->GetActorLocation(), GetActorLocation());

					float dF = FVector::Dist(foundResources[i]->GetActorLocation(), GetActorLocation());

					if (dR > dF) {
						Resource = r;
					}
				}
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