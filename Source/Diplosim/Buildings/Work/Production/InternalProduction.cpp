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
}

void AInternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

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
		SetTimer(Citizen);
	else {
		AlterTimer();

		PauseTimer(false);
	}
}

void AInternalProduction::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (GetCitizensAtBuilding().IsEmpty())
		PauseTimer(true);
	else
		AlterTimer();
}

void AInternalProduction::Production(ACitizen* Citizen)
{
	Super::Production(Citizen);

	GetCitizensAtBuilding()[0]->Carry(Camera->ResourceManager->GetResources(this)[0]->GetDefaultObject<AResource>(), FMath::RandRange(MinYield, MaxYield), this);

	StoreResource(Citizen);

	for (ACitizen* citizen : GetCitizensAtBuilding())
		Camera->CitizenManager->Injure(citizen, 99);

	SetTimer(GetCitizensAtBuilding()[0]);

	for (const FItemStruct item : Intake)
		if (item.Use > item.Stored)
			for (ACitizen* citizen : GetCitizensAtBuilding())
				CheckStored(citizen, Intake);
}

float AInternalProduction::GetTime()
{
	float time = TimeLength / GetCitizensAtBuilding().Num();

	for (ACitizen* citizen : GetCitizensAtBuilding())
		time -= (time / GetCitizensAtBuilding().Num()) * (FMath::LogX(citizen->MovementComponent->InitialSpeed, citizen->MovementComponent->MaxSpeed) * citizen->GetProductivity() - 1.0f);

	return time;
}

void AInternalProduction::SetTimer(ACitizen* Citizen)
{
	FTimerStruct timer;
	timer.CreateTimer("Internal", this, GetTime(), FTimerDelegate::CreateUObject(this, &AInternalProduction::Production, Citizen), false);

	Camera->CitizenManager->Timers.Add(timer);
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