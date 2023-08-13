#include "Production.h"

#include "Player/Camera.h"
#include "Player/ResourceManager.h"
#include "AI/Citizen.h"

AProduction::AProduction()
{
	Storage = 0;
	StorageCap = 1000;

	Resource = nullptr;
}

void AProduction::Store(int32 Amount, ACitizen* Citizen)
{
	int32 quantity = (Storage + Amount);
	if (0 <= quantity && quantity <= StorageCap) {
		Storage += Amount;

		Camera->ResourceManagerComponent->ChangeResource(Resource, Amount);

		Production(Citizen);
	}
	else {
		GetWorldTimerManager().ClearTimer(ProdTimer);

		FTimerHandle StoreCheckTimer;
		GetWorldTimerManager().SetTimer(StoreCheckTimer, FTimerDelegate::CreateUObject(this, &AProduction::Store, Amount, Citizen), 30.0f, false);
	}
}

void AProduction::Production(ACitizen* Citizen)
{

}