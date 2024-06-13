#include "InternalProduction.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"

AInternalProduction::AInternalProduction()
{
	MinYield = 1;
	MaxYield = 5;

	TimeLength = 10.0f;
}

void AInternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	int32 numAtWork = GetCitizensAtBuilding().Num();

	if (numAtWork == 1)
		Production(Citizen);
}

void AInternalProduction::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	int32 numAtWork = GetCitizensAtBuilding().Num();

	if (numAtWork == 0)
		GetWorldTimerManager().ClearTimer(ProdTimer);
}

void AInternalProduction::Production(ACitizen* Citizen)
{
	Super::Production(Citizen);

	int32 numAtWork = GetCitizensAtBuilding().Num();

	if (!GetWorldTimerManager().IsTimerActive(ProdTimer))
		GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AInternalProduction::Produce, Citizen), (TimeLength / 100.0f / numAtWork), false);
}

void AInternalProduction::Produce(ACitizen* Citizen)
{
	PercentageDone += 1;

	if (PercentageDone == 100) {
		Citizen->Carry(Camera->ResourceManagerComponent->GetResource(this)->GetDefaultObject<AResource>(), FMath::RandRange(MinYield, MaxYield), this);

		StoreResource(Citizen);

		PercentageDone = 0;
	}

	if (Citizen->Building.BuildingAt == this)
		Production(Citizen);
}