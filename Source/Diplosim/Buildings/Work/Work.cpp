#include "Work.h"

#include "Blueprint/UserWidget.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Production/ExternalProduction.h"
#include "Buildings/Work/Booster.h"
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
	WagePerHour = 0.0f;

	bCanAttendEvents = true;
	bEmergency = false;

	ForcefieldRange = 0;
	Boosters = 0;
}

void AWork::BeginPlay()
{
	Super::BeginPlay();

	WorkHours.Empty();

	for (int32 i = 0; i < MaxCapacity; i++) {
		FWorkHoursStruct hours;

		for (int32 j = 0; j < 24; j++) {
			EWorkType type = EWorkType::Freetime;

			if (j >= 6 && j < 18)
				type = EWorkType::Work;

			hours.WorkHours.Add(j, type);
		}

		WorkHours.Add(hours);
	}

	CheckWorkStatus(Camera->Grid->AtmosphereComponent->Calendar.Hour);
}

bool AWork::AddCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.Employment = this;

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

	Citizen->Building.Employment = nullptr;

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
	for (int32 i = 0; i < WorkHours.Num(); i++) {
		if (IsValid(WorkHours[i].Citizen))
			continue;

		if (bAdd)
			WorkHours[i].Citizen = Citizen;
		else
			WorkHours[i].Citizen = nullptr;

		if (Camera->HoursUIInstance->IsInViewport())
			Camera->UpdateWorkHours(this, i);

		break;
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

	FWorkHoursStruct hours;
	hours.Citizen = Citizen;

	int32 index = WorkHours.Find(hours);

	EWorkType type = *WorkHours[index].WorkHours.Find(Hour);

	if ((type == EWorkType::Work && Camera->PoliticsManager->GetRaidPolicyStatus(Citizen) == ERaidPolicy::Default && !Camera->EventsManager->IsAttendingEvent(Citizen) && !Citizen->bHolliday) || bEmergency)
		return true;

	return false;
}

bool AWork::IsAtWork(ACitizen* Citizen)
{
	if (Citizen->Building.BuildingAt == this)
		return true;
	else if (IsA<AExternalProduction>()) {
		AExternalProduction* externalProduction = Cast<AExternalProduction>(this);

		for (auto& element : externalProduction->GetValidResources()) {
			for (FWorkerStruct workerStruct : element.Key->WorkerStruct) {
				if (!workerStruct.Citizens.Contains(Citizen))
					continue;

				return true;
			}
		}
	}

	return false;
}

int32 AWork::GetHoursInADay(ACitizen* Citizen)
{
	int32 hours = 0;

	for (FWorkHoursStruct hoursStruct : WorkHours) {
		if (hoursStruct.Citizen != Citizen)
			continue;

		for (auto& element : hoursStruct.WorkHours)
			if (element.Value == EWorkType::Work)
				hours++;
	}

	return hours;
}

int32 AWork::GetWage(ACitizen* Citizen)
{
	return WagePerHour * GetHoursInADay(Citizen);
}

int32 AWork::GetAverageWage()
{
	int32 averageHours = 0;

	for (FWorkHoursStruct hoursStruct : WorkHours)
		for (auto& element : hoursStruct.WorkHours)
			if (element.Value == EWorkType::Work)
				averageHours++;

	averageHours /= Capacity;

	return WagePerHour * averageHours;
}

void AWork::SetNewWorkHours(int32 Index, FWorkHoursStruct NewWorkHours)
{
	WorkHours[Index].WorkHours = NewWorkHours.WorkHours;
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
	TArray<ACitizen*> citizens = GetCitizensAtBuilding();

	for (ACitizen* citizen : citizens) {
		int32 chance = Camera->Stream.RandRange(1, 100);

		if (chance > 98)
			continue;

		citizen->HealthComponent->TakeHealth(5, this);
	}

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