#include "Buildings/Work/Work.h"

#include "Blueprint/UserWidget.h"
#include "NiagaraComponent.h"

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
#include "Player/Managers/PoliceManager.h"
#include "Universal/HealthComponent.h"

AWork::AWork()
{
	WorkHat = nullptr;

	bCanAttendEvents = true;
	bEmergency = false;

	bGradualShutdown = false;
	GradualShutdownCounter = 0;

	ForcefieldRange = 0;
	Boosters = 0;
}

bool AWork::AddCitizen(ACitizen* Citizen)
{
	if (!Super::AddCitizen(Citizen))
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
	if (!Super::RemoveCitizen(Citizen))
		return false;

	Citizen->BuildingComponent->Employment = nullptr;

	AddToWorkHours(Citizen, false);

	Citizen->AIController->DefaultAction();

	return true;
}

void AWork::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	if (!IsWorking(Citizen))
		Citizen->AIController->DefaultAction();

	if (!Camera->Grid->AIVisualiser->DoesCitizenHaveHat(Citizen))
		Camera->Grid->AIVisualiser->AddCitizenToHISMHat(Citizen, WorkHat);
}

void AWork::AddToWorkHours(ACitizen* Citizen, bool bAdd)
{
	if (bAdd) {
		FCapacityStruct* capacityStruct = GetBestWorkHours(Citizen);
		capacityStruct->Citizen = Citizen;
	}
	else {
		FCapacityStruct capacityStruct;
		capacityStruct.Citizen = Citizen;

		int32 index = Occupied.Find(capacityStruct);

		Occupied[index].Citizen = nullptr;
	}
}

void AWork::CheckWorkStatus(int32 Hour)
{
	GradualShutdown();

	for (ACitizen* citizen : GetOccupied()) {
		bool isWorkingNow = IsWorking(citizen, Hour);
		bool isAtWork = IsAtWork(citizen);

		if ((isWorkingNow && !isAtWork) || (!isWorkingNow && isAtWork)) {
			citizen->AIController->DefaultAction();

			if (!isWorkingNow && isAtWork && IsA<AExternalProduction>())
				Cast<AExternalProduction>(this)->RemoveWorkerFromResource(citizen);
		}
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

	if ((type == EWorkType::Work && Camera->PoliticsManager->GetRaidPolicyStatus(Citizen) == ERaidPolicy::Default && !Camera->EventsManager->IsAttendingEvent(Citizen) && !Camera->EventsManager->IsHoliday(Citizen) && !IsCapacityFull()) || bEmergency)
		return true;

	return false;
}

void AWork::GradualShutdown()
{
	if (!bGradualShutdown)
		return;

	if (GetCitizensAtBuilding().IsEmpty())
		GradualShutdownCounter++;
	else
		GradualShutdownCounter = 0;

	if (GradualShutdownCounter != 24)
		return;

	ParticleComponent->Deactivate();
}

bool AWork::IsAtWork(ACitizen* Citizen)
{
	if (Citizen->BuildingComponent->BuildingAt == this || Citizen->AIController->MoveRequest.GetGoalActor() == this || (IsA(Camera->PoliceManager->PoliceStationClass) && Camera->PoliceManager->IsInAPoliceReport(Citizen, Camera->ConquestManager->GetFaction("", Citizen))))
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

		if (index == INDEX_NONE)
			CapacityStruct = GetBestWorkHours(Citizen);
		else
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

void AWork::UpdateWorkHours(int32 Index, int32 Hour, EWorkType WorkType)
{
	Occupied[Index].WorkHours.Add(Hour, WorkType);

	if (Hour == Camera->Grid->AtmosphereComponent->Calendar.Hour)
		CheckWorkStatus(Hour);
}

TMap<int32, EWorkType> AWork::GetWorkHours(int32 Index)
{
	return Occupied[Index].WorkHours;
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

	Boosters = 0;

	for (ABuilding* building : faction->Buildings) {
		if (!building->IsA<ABooster>())
			continue;

		if (Cast<ABooster>(building)->GetAffectedBuildings().Contains(this))
			Boosters++;
	}
}