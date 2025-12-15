#include "InternalProduction.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Map/Grid.h"

AInternalProduction::AInternalProduction()
{
	PrimaryActorTick.bCanEverTick = false;

	MinYield = 1.0f;
	MaxYield = 5.0f;

	TimeLength = 10;

	bMultiplicitive = false;

	bNoTimer = false;
}

void AInternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen) || bNoTimer)
		return;

	if (IsCapacityFull())
		return;

	for (FItemStruct item : Intake) {
		if (item.Use > item.Stored) {
			CheckStored(Citizen, Intake);

			return;
		}
	}

	FTimerStruct* timer = Camera->TimerManager->FindTimer("Internal", this);

	if (timer == nullptr)
		SetTimer();
	else {
		AlterTimer();

		PauseTimer(false);
	}
}

void AInternalProduction::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (!GetOccupied().Contains(Citizen) || bNoTimer)
		return;

	if (GetCitizensAtBuilding().IsEmpty())
		PauseTimer(true);
	else
		AlterTimer();
}

void AInternalProduction::Production(ACitizen* Citizen)
{
	Super::Production(Citizen);

	TArray<TSubclassOf<AResource>> resources = Camera->ResourceManager->GetResources(this);

	if (!resources.IsEmpty()) {
		float yield = MinYield;

		if (MinYield != MaxYield)
			yield = Camera->Grid->Stream.RandRange(MinYield, MaxYield);

		if (bMultiplicitive) {
			yield = FMath::CeilToInt32(Camera->ResourceManager->GetResourceAmount(FactionName, resources[0]) * yield);

			Camera->ResourceManager->AddUniversalResource(Camera->ConquestManager->GetFaction(FactionName), resources[0], yield);
		}
		else {
			GetCitizensAtBuilding()[0]->Carry(resources[0]->GetDefaultObject<AResource>(), yield, this);

			StoreResource(GetCitizensAtBuilding()[0]);
		}
	}

	SetTimer();

	for (const FItemStruct item : Intake)
		if (item.Use > item.Stored)
			for (ACitizen* citizen : GetCitizensAtBuilding())
				CheckStored(citizen, Intake);
}

float AInternalProduction::GetTime()
{
	float time = TimeLength;

	for (ACitizen* citizen : GetCitizensAtBuilding())
		time -= (time / GetCitizensAtBuilding().Num()) * citizen->GetProductivity() - 1.0f;

	return time;
}

void AInternalProduction::SetTimer()
{
	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(GetCitizensAtBuilding()[0], params);
	Camera->TimerManager->CreateTimer("Internal", this, GetTime(), "Production", params, false);
}

void AInternalProduction::AlterTimer()
{
	FTimerStruct* timer = Camera->TimerManager->FindTimer("Internal", this);

	if (timer == nullptr)
		return;

	timer->Target = GetTime();
}

void AInternalProduction::PauseTimer(bool bPause)
{
	FTimerStruct* timer = Camera->TimerManager->FindTimer("Internal", this);

	if (timer == nullptr)
		return;

	timer->bPaused = bPause;
}