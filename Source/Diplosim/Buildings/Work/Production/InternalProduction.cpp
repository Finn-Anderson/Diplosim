#include "InternalProduction.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/AIMovementComponent.h"

AInternalProduction::AInternalProduction()
{
	PrimaryActorTick.bCanEverTick = false;

	MinYield = 1;
	MaxYield = 5;

	TimeLength = 10;

	bNoTimer = false;
}

void AInternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen) || bNoTimer)
		return;

	for (FItemStruct item : Intake) {
		if (item.Use > item.Stored) {
			CheckStored(Citizen, Intake);

			return;
		}
	}

	int32 amount = 0;

	for (FItemStruct item : Storage)
		amount += item.Amount;

	if (amount == StorageCap)
		return;

	FTimerStruct* timer = Camera->CitizenManager->FindTimer("Internal", this);

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

	if (!Camera->ResourceManager->GetResources(this).IsEmpty()) {
		GetCitizensAtBuilding()[0]->Carry(Camera->ResourceManager->GetResources(this)[0]->GetDefaultObject<AResource>(), FMath::RandRange(MinYield, MaxYield), this);

		StoreResource(GetCitizensAtBuilding()[0]);
	}

	for (ACitizen* citizen : GetCitizensAtBuilding())
		Camera->CitizenManager->Injure(citizen, 99);

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
	Camera->CitizenManager->CreateTimer("Internal", this, GetTime(), FTimerDelegate::CreateUObject(this, &AInternalProduction::Production, GetCitizensAtBuilding()[0]), false);
}

void AInternalProduction::AlterTimer()
{
	FTimerStruct* timer = Camera->CitizenManager->FindTimer("Internal", this);

	if (timer == nullptr)
		return;

	timer->Target = GetTime();
}

void AInternalProduction::PauseTimer(bool bPause)
{
	FTimerStruct* timer = Camera->CitizenManager->FindTimer("Internal", this);

	if (timer == nullptr)
		return;

	timer->bPaused = bPause;
}