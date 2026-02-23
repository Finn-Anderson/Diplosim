#include "Buildings/Work/Work.h"

#include "Blueprint/UserWidget.h"

#include "AI/DiplosimAIController.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "Buildings/Work/Booster.h"
#include "Buildings/Work/Production/ExternalProduction.h"
#include "Buildings/Work/Service/School.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/EventsManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Universal/HealthComponent.h"

AWork::AWork()
{
	bCanAttendEvents = true;
	bEmergency = false;

	ForcefieldRange = 0;
	Boosters = 0;
}

bool AWork::AddCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->BuildingComponent->Employment = this;

	AddToWorkHours(Citizen, true); 

	if (IsWorking(Citizen)) {
		bool bAttendingEvent = Camera->EventsManager->IsAttendingEvent(Citizen);

		if (bCanAttendEvents && bAttendingEvent)
			return true;
		else if (bAttendingEvent)
			Camera->EventsManager->RemoveFromEvent(Citizen);

		Citizen->AIController->DefaultAction();
	}

	return true;
}

bool AWork::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->BuildingComponent->Employment = nullptr;

	AddToWorkHours(Citizen, false);

	Camera->Grid->AIVisualiser->RemoveCitizenFromHISMHat(Citizen);

	Citizen->AIController->DefaultAction();

	return true;
}

void AWork::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!IsWorking(Citizen) && GetOccupied().Contains(Citizen))
		Citizen->AIController->DefaultAction();

	if (GetOccupied().Contains(Citizen) && !Camera->Grid->AIVisualiser->DoesCitizenHaveHat(Citizen))
		Camera->Grid->AIVisualiser->AddCitizenToHISMHat(Citizen, WorkHat);
}

void AWork::AddToWorkHours(ACitizen* Citizen, bool bAdd)
{
	int32 index = INDEX_NONE;

	if (bAdd) {
		FCapacityStruct* capacityStruct = GetBestWorkHours(Citizen);
		capacityStruct->Citizen = Citizen;

		index = Occupied.Find(*capacityStruct);
	}
	else {
		FCapacityStruct capacityStruct;
		capacityStruct.Citizen = Citizen;

		index = Occupied.Find(capacityStruct);

		Occupied[index].Citizen = nullptr;
	}
}

void AWork::CheckWorkStatus(int32 Hour)
{
	if (IsA<ASchool>() && !GetCitizensAtBuilding().IsEmpty())
		Cast<ASchool>(this)->AddProgress();

	for (ACitizen* citizen : GetOccupied()) {
		bool isWorkingNow = IsWorking(citizen, Hour);

		if ((isWorkingNow && !IsAtWork(citizen)) || (!isWorkingNow && IsAtWork(citizen)))
			citizen->AIController->DefaultAction();
	}
}

bool AWork::IsWorking(ACitizen* Citizen, int32 Hour)
{
	if (Hour == -1)
		Hour = Camera->Grid->AtmosphereComponent->Calendar.Hour;

	FCapacityStruct capacityStruct;
	capacityStruct.Citizen = Citizen;

	int32 index = Occupied.Find(capacityStruct);

	EWorkType type = *Occupied[index].WorkHours.Find(Hour);

	if ((type == EWorkType::Work && Camera->PoliticsManager->GetRaidPolicyStatus(Citizen) == ERaidPolicy::Default && !Camera->EventsManager->IsAttendingEvent(Citizen) && !Camera->EventsManager->IsHolliday(Citizen)) || bEmergency)
		return true;

	return false;
}

bool AWork::IsAtWork(ACitizen* Citizen)
{
	if (Citizen->BuildingComponent->BuildingAt == this)
		return true;

	return false;
}

//
// Wage + Hours
//
void AWork::InitialiseCapacityStruct()
{
	for (int32 i = 0; i < GetCapacity(); i++) {
		FCapacityStruct capacityStruct;
		capacityStruct.Amount = Camera->ConquestManager->GetBuildingClassAmount(FactionName, GetClass());

		Occupied.Add(capacityStruct);
	}
}

int32 AWork::GetHoursInADay(ACitizen* Citizen, FCapacityStruct* CapacityStruct)
{
	int32 hours = 0;

	if (CapacityStruct == nullptr) {
		FCapacityStruct capStruct;
		capStruct.Citizen = Citizen;

		int32 index = Occupied.Find(capStruct);

		CapacityStruct = &Occupied[index];
	}

	for (auto& element : CapacityStruct->WorkHours)
		if (element.Value == EWorkType::Work)
			hours++;

	return hours;
}

int32 AWork::GetWage(ACitizen* Citizen)
{
	return GetAmount(Citizen) * GetHoursInADay(Citizen);
}

int32 AWork::GetAverageWage()
{
	int32 averageHours = 0;
	int32 averageWage = 0;

	for (FCapacityStruct capacityStruct : Occupied) {
		averageWage += capacityStruct.Amount;

		for (auto& element : capacityStruct.WorkHours)
			if (element.Value == EWorkType::Work)
				averageHours++;
	}

	averageHours /= Capacity;
	averageWage /= Capacity;

	return averageWage * averageHours;
}

FCapacityStruct* AWork::GetBestWorkHours(ACitizen* Citizen)
{
	FCapacityStruct* bestCapacityStruct = nullptr;

	for (FCapacityStruct& capacityStruct : Occupied) {
		if (IsValid(capacityStruct.Citizen) || capacityStruct.bBlocked)
			continue;

		int32 h = GetHoursInADay(nullptr, &capacityStruct);

		if (h < Citizen->BuildingComponent->IdealHoursWorkedMin || h > Citizen->BuildingComponent->IdealHoursWorkedMax)
			continue;

		if (bestCapacityStruct == nullptr) {
			bestCapacityStruct = &capacityStruct;

			continue;
		}

		if (bestCapacityStruct->Amount < capacityStruct.Amount)
			bestCapacityStruct = &capacityStruct;
	}

	return bestCapacityStruct;
}

void AWork::SetNewWorkHours(int32 Index, TMap<int32, EWorkType> NewWorkHours)
{
	Occupied[Index].WorkHours = NewWorkHours;

	CheckWorkStatus(Camera->Grid->AtmosphereComponent->Calendar.Hour);
}

void AWork::ResetWorkHours()
{
	for (FCapacityStruct& capacityStruct : Occupied) {
		capacityStruct.Amount = DefaultAmount;
		capacityStruct.bBlocked = false;
		capacityStruct.ResetWorkHours();
	}

	CheckWorkStatus(Camera->Grid->AtmosphereComponent->Calendar.Hour);
}

void AWork::SetEmergency(bool bStatus)
{
	if (bStatus == bEmergency)
		return;

	bEmergency = bStatus;

	CheckWorkStatus(Camera->Grid->AtmosphereComponent->Calendar.Hour);
}

void AWork::Production(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	for (ABuilding* building : faction->Buildings) {
		if (!building->IsA<ABooster>())
			continue;

		if (Cast<ABooster>(building)->GetAffectedBuildings().Contains(this))
			Boosters++;
	}
}

bool AWork::IsCapacityFull()
{
	for (auto& element : Camera->ResourceManager->GetBuildingCapacities(this)) {
		FItemStruct item;
		item.Resource = element.Key;

		int32 index = Storage.Find(item);

		if (index == INDEX_NONE)
			continue;

		if (Storage[index].Amount == element.Value)
			return true;
	}

	return false;
}