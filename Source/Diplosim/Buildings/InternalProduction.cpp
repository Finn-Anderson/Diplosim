#include "InternalProduction.h"

#include "Resource.h"
#include "Player/Camera.h"

AInternalProduction::AInternalProduction()
{
	TimeLength = 60.0f;
}

void AInternalProduction::Enter(ACitizen* Citizen)
{
	if (Occupied.Contains(Citizen)) {
		AtWork.Add(Citizen);

		if (AtWork.Num() == 1) {
			Production(Citizen);
		}
	}
}

void AInternalProduction::Leave(ACitizen* Citizen)
{
	if (AtWork.Contains(Citizen)) {
		AtWork.Remove(Citizen);

		if (AtWork.Num() == 0) {
			GetWorldTimerManager().ClearTimer(ProdTimer);
		}
	}
}

void AInternalProduction::Production(ACitizen* Citizen)
{
	GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AInternalProduction::ProductionDone, Citizen), (TimeLength / AtWork.Num()), false);
}

void AInternalProduction::ProductionDone(ACitizen* Citizen)
{
	if (Storage < StorageCap) {
		AResource* r = GetWorld()->SpawnActor<AResource>(Resource);

		Store(r->GetYield(), Citizen);

		r->Destroy();
	}
}