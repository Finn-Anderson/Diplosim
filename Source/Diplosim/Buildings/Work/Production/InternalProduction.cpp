#include "InternalProduction.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"

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

	FTimerStruct timer;
	timer.CreateTimer(this, TimeLength, FTimerDelegate::CreateUObject(this, &AInternalProduction::Production, Citizen), false);

	if (!Camera->CitizenManager->Timers.Contains(timer))
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
	GetCitizensAtBuilding()[0]->Carry(Camera->ResourceManager->GetResources(this)[0]->GetDefaultObject<AResource>(), FMath::RandRange(MinYield, MaxYield), this);

	for (ACitizen* citizen : GetCitizensAtBuilding())
		Camera->CitizenManager->Injure(citizen);

	SetTimer(GetCitizensAtBuilding()[0]);

	Super::Production(GetCitizensAtBuilding()[0]);

	for (FItemStruct item : Intake) {
		if (item.Use > item.Stored) {
			for (ACitizen* citizen : GetCitizensAtBuilding())
				CheckStored(citizen, Intake);

			SetActorTickEnabled(false);
		}
	}
}

void AInternalProduction::SetTimer(ACitizen* Citizen)
{
	float tally = 1;

	for (ACitizen* citizen : GetCitizensAtBuilding())
		tally *= FMath::LogX(citizen->InitialSpeed, citizen->GetCharacterMovement()->MaxWalkSpeed);

	float time = TimeLength / GetCitizensAtBuilding().Num() / tally;

	FTimerStruct timer;
	timer.CreateTimer(this, time, FTimerDelegate::CreateUObject(this, &AInternalProduction::Production, Citizen), false);

	Camera->CitizenManager->Timers.Add(timer);
}

void AInternalProduction::AlterTimer()
{
	FTimerStruct timer;
	timer.CreateTimer(this, TimeLength, FTimerDelegate::CreateUObject(this, &AInternalProduction::Production, GetCitizensAtBuilding()[0]), false);

	int32 index = Camera->CitizenManager->Timers.Find(timer);

	if (index == INDEX_NONE)
		return;

	float tally = 1;

	for (ACitizen* citizen : GetCitizensAtBuilding())
		tally *= FMath::LogX(citizen->InitialSpeed, citizen->GetCharacterMovement()->MaxWalkSpeed);

	float time = TimeLength / GetCitizensAtBuilding().Num() / tally;

	Camera->CitizenManager->Timers[index].Target = time;
}

void AInternalProduction::PauseTimer(bool bPause)
{
	FTimerStruct timer;
	timer.CreateTimer(this, TimeLength, FTimerDelegate::CreateUObject(this, &AInternalProduction::Production, GetCitizensAtBuilding()[0]), false);

	int32 index = Camera->CitizenManager->Timers.Find(timer);

	if (index == INDEX_NONE)
		return;

	Camera->CitizenManager->Timers[index].bPaused = bPause;
}