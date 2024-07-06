#include "InternalProduction.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"

AInternalProduction::AInternalProduction()
{
	MinYield = 1;
	MaxYield = 5;

	TimeLength = 10.0f;
}

void AInternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (GetCitizensAtBuilding().Num() == 1)
		Production(Citizen);
}

void AInternalProduction::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (GetCitizensAtBuilding().IsEmpty() && GetWorldTimerManager().IsTimerActive(ProdTimer))
		GetWorldTimerManager().PauseTimer(ProdTimer);
}

void AInternalProduction::Production(ACitizen* Citizen)
{
	Super::Production(Citizen);

	if (GetWorldTimerManager().IsTimerPaused(ProdTimer))
		GetWorldTimerManager().UnPauseTimer(ProdTimer);
	else if (!GetWorldTimerManager().IsTimerActive(ProdTimer))
		GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AInternalProduction::Produce, Citizen), (TimeLength / GetCitizensAtBuilding().Num()), false);
}

void AInternalProduction::Produce(ACitizen* Citizen)
{
	Citizen->Carry(Camera->ResourceManager->GetResource(this)->GetDefaultObject<AResource>(), FMath::RandRange(MinYield, MaxYield), this);

	StoreResource(Citizen);

	Production(Citizen);
}