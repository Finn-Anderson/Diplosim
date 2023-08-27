#include "InternalProduction.h"

#include "Resource.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"

AInternalProduction::AInternalProduction()
{
	TimeLength = 10.0f;
}

void AInternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (AtWork.Num() == 1) {
		Store(0, Citizen);
	}
}

void AInternalProduction::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (AtWork.Num() == 0) {
		GetWorldTimerManager().ClearTimer(ProdTimer);
	}
}

void AInternalProduction::Production(ACitizen* Citizen)
{
	if (!GetWorldTimerManager().IsTimerActive(ProdTimer)) {
		GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AInternalProduction::Produce, Citizen), (TimeLength / AtWork.Num()), false);
	}
}

void AInternalProduction::Produce(ACitizen* Citizen)
{
	PercentageDone += 1;

	if (PercentageDone == 100) {
		ProductionDone(Citizen);

		PercentageDone = 0;
	}

	if (AtWork.Contains(Citizen)) {
		GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AInternalProduction::Produce, Citizen), (TimeLength / AtWork.Num()), false);
	}
}

void AInternalProduction::ProductionDone(ACitizen* Citizen)
{
	AResource* r = GetWorld()->SpawnActor<AResource>(Camera->ResourceManagerComponent->GetResource(this));

	Store(r->GetYield(), Citizen);

	r->Destroy();
}