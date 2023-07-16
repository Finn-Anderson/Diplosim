#include "InternalProduction.h"

#include "Resource.h"
#include "Camera.h"

void AInternalProduction::Production(ACitizen* Citizen)
{
	GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AInternalProduction::ProductionDone, Citizen), (TimeLength / AtWork.Num()), false);
}

void AInternalProduction::ProductionDone(ACitizen* Citizen)
{
	if (Storage < StorageCap) {
		AResource* r = GetWorld()->SpawnActor<AResource>(ActorToGetResource);

		Store(r->GetYield(), Citizen);

		r->Destroy();
	}
}