#include "Production.h"

#include "Camera.h"
#include "ResourceManager.h"
#include "Citizen.h"

AProduction::AProduction()
{
	Storage = 0;
	StorageCap = 1000;

	Resource = nullptr;
}

void AProduction::Store(int32 Amount, ACitizen* Citizen)
{
	if (0 < (Storage + Amount) && (Storage + Amount) < StorageCap) {
		Storage += Amount;

		Camera->ResourceManager->ChangeResource(Resource, Amount);

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