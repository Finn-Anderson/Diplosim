#include "Production.h"

#include "Camera.h"
#include "ResourceManager.h"
#include "Citizen.h"

AProduction::AProduction()
{
	Storage = 0;
	StorageCap = 1000;

	TimeLength = 60.0f;

	ActorToGetResource = nullptr;
}

void AProduction::Action(ACitizen* Citizen)
{
	if (Occupied.Contains(Citizen)) {
		if (AtWork.Contains(Citizen)) {
			AtWork.Remove(Citizen);
		}
		else {
			AtWork.Add(Citizen);

			if (Citizen->Carrying > 0) {
				Store(Citizen->Carrying, Citizen);

				Citizen->Carrying = 0;
			}
			else if (AtWork.Num() == 1) {
				Store(0, Citizen);
			}
		}
	}

	if (AtWork.Num() == 0) {
		GetWorldTimerManager().ClearTimer(ProdTimer);
	}
}

void AProduction::Store(int32 Amount, ACitizen* Citizen)
{
	if ((Storage + Amount) < StorageCap) {
		Storage += Amount;

		Camera->ResourceManager->ChangeResource(Produce, Amount);

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