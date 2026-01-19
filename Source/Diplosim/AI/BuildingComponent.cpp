#include "AI/BuildingComponent.h"

#include "AI/Citizen.h"
#include "AI/AIMovementComponent.h"
#include "AI/DiplosimAIController.h"
#include "AI/HappinessComponent.h"
#include "AI/BioComponent.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Buildings/House.h"
#include "Buildings/Work/Work.h"
#include "Buildings/Work/Booster.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Service/Orphanage.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/DiplosimTimerManager.h"

UBuildingComponent::UBuildingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	House = nullptr;
	Employment = nullptr;
	School = nullptr;
	Orphanage = nullptr;
	BuildingAt = nullptr;
	EnterLocation = FVector::Zero();

	TimeOfAcquirement.Init(-1000.0f, 3);
	FoundHouseOccupant = nullptr;

	IdealHoursWorkedMin = 4;
	IdealHoursWorkedMax = 12;
}

//
// Find Job, House and Education
//
void UBuildingComponent::FindEducation(ASchool* Education, int32 TimeToCompleteDay)
{
	if (!IsValid(AllocatedBuildings[0]))
		SetAcquiredTime(0, -1000.0f);

	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	FFactionStruct* faction = citizen->Camera->ConquestManager->GetFaction("", citizen);

	if (HasRecentlyAcquired(0, TimeToCompleteDay) || citizen->BioComponent->EducationLevel == 5 || !citizen->CanAffordEducationLevel() || Education->GetOccupied().IsEmpty())
		return;

	if (citizen->BioComponent->Age >= citizen->Camera->PoliticsManager->GetLawValue(faction->Name, "Work Age") || citizen->BioComponent->Age < citizen->Camera->PoliticsManager->GetLawValue(faction->Name, "Education Age"))
		return;

	bool bFull = true;

	for (ACitizen* occupant : Education->GetCitizensAtBuilding()) {
		int32 studentsNum = Education->GetStudentsAtSchool(occupant).Num();

		if (studentsNum < Education->Space) {
			bFull = false;

			break;
		}
	}

	if (bFull)
		return;

	ABuilding* chosenSchool = AllocatedBuildings[0];

	if (!IsValid(chosenSchool)) {
		AllocatedBuildings[0] = Education;
	}
	else {
		FVector location = citizen->MovementComponent->Transform.GetLocation();

		if (IsValid(AllocatedBuildings[2]))
			location = AllocatedBuildings[2]->GetActorLocation();

		double magnitude = citizen->AIController->GetClosestActor(400.0f, location, chosenSchool->GetActorLocation(), Education->GetActorLocation(), true, 2, 1);

		if (magnitude <= 0.0f)
			return;

		AllocatedBuildings[0] = Education;
	}
}

void UBuildingComponent::FindJob(AWork* Job, int32 TimeToCompleteDay)
{
	if (!IsValid(AllocatedBuildings[1]))
		SetAcquiredTime(1, -1000.0f);

	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	if (HasRecentlyAcquired(1, TimeToCompleteDay) || Job->GetCapacity() == Job->GetOccupied().Num() || !CanWork(Job) || !WillWork())
		return;

	AWork* chosenWorkplace = Cast<AWork>(AllocatedBuildings[1]);

	if (!IsValid(chosenWorkplace)) {
		AllocatedBuildings[1] = Job;
	}
	else {
		FCapacityStruct* capacityStruct = Job->GetBestWorkHours(citizen);

		if (capacityStruct == nullptr)
			return;

		int32 diff = (capacityStruct->Amount * Job->GetHoursInADay(nullptr, capacityStruct)) - chosenWorkplace->GetWage(citizen);

		int32* happiness = citizen->HappinessComponent->Modifiers.Find("Work Happiness");

		if (happiness != nullptr)
			diff -= *happiness / 5;

		FVector location = citizen->MovementComponent->Transform.GetLocation();

		if (IsValid(AllocatedBuildings[1]))
			location = AllocatedBuildings[1]->GetActorLocation();

		int32 currentValue = 1;
		int32 newValue = 1;

		if (diff < 0)
			currentValue += FMath::Abs(diff);
		else if (diff > 0)
			newValue += diff;

		auto magnitude = citizen->AIController->GetClosestActor(400.0f, location, chosenWorkplace->GetActorLocation(), Job->GetActorLocation(), true, currentValue, newValue);

		if (magnitude <= 0.0f)
			return;

		AllocatedBuildings[1] = Job;
	}
}

void UBuildingComponent::FindHouse(AHouse* NewHouse, int32 TimeToCompleteDay, TArray<ACitizen*> Roommates)
{
	if (!IsValid(AllocatedBuildings[2]))
		SetAcquiredTime(2, -1000.0f);

	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	if (HasRecentlyAcquired(2, TimeToCompleteDay))
		return;

	int32 wages = 0;
	ACitizen* occupant = nullptr;

	if (IsValid(Employment))
		wages = Employment->GetWage(citizen);
	else
		wages = citizen->Balance;

	if (Roommates.Num() == 0) {
		for (ACitizen* occupier : NewHouse->GetOccupied()) {
			TArray<ACitizen*> citizens = House->GetVisitors(occupier);

			if (citizens.Num() == NewHouse->Space || occupier->BioComponent->Partner != nullptr)
				continue;

			citizens.Add(occupier);

			bool bAvailable = true;

			for (ACitizen* c : citizens) {
				int32 likeness = 0;

				c->Camera->CitizenManager->PersonalityComparison(citizen, c, likeness);

				if (likeness < 0) {
					bAvailable = false;

					break;
				}

				if (IsValid(c->BuildingComponent->Employment))
					wages += c->BuildingComponent->Employment->GetWage(c);
				else
					wages += c->Balance;
			}

			if (bAvailable) {
				occupant = occupier;

				break;
			}
		}
	}

	int32 newRent = NewHouse->GetBestAvailableRoom()->Amount;
	int32 oldRent = Cast<AHouse>(AllocatedBuildings[2])->GetAmount(citizen);

	if (wages < newRent || NewHouse->Space < Roommates.Num() || (!IsValid(occupant) && NewHouse->GetOccupied().Num() == NewHouse->GetCapacity()))
		return;

	AHouse* chosenHouse = Cast<AHouse>(AllocatedBuildings[2]);

	if (!IsValid(chosenHouse) || (IsValid(FoundHouseOccupant) && !IsValid(occupant))) {
		AllocatedBuildings[2] = NewHouse;
		FoundHouseOccupant = occupant;
	}
	else {
		int32 newLeftoverMoney = (wages - newRent) * 50;

		wages -= oldRent;
		wages *= 50;

		int32 currentValue = FMath::Max(chosenHouse->GetSatisfactionLevel(oldRent) / 10 + chosenHouse->BaseRent + wages, 0);
		int32 newValue = FMath::Max(NewHouse->GetSatisfactionLevel(newRent) / 10 + NewHouse->BaseRent + newLeftoverMoney, 0);

		FVector workLocation = citizen->MovementComponent->Transform.GetLocation();

		if (IsValid(Employment))
			workLocation = Employment->GetActorLocation();

		double magnitude = citizen->AIController->GetClosestActor(400.0f, workLocation, chosenHouse->GetActorLocation(), NewHouse->GetActorLocation(), true, currentValue, newValue);

		if (citizen->BioComponent->Partner != nullptr && IsValid(citizen->BioComponent->Partner->BuildingComponent->Employment)) {
			FVector partnerWorkLoc = citizen->BioComponent->Partner->BuildingComponent->Employment->GetActorLocation();

			double m = citizen->AIController->GetClosestActor(400.0f, partnerWorkLoc, chosenHouse->GetActorLocation(), NewHouse->GetActorLocation(), true, currentValue, newValue);

			magnitude += m;
		}

		if (magnitude <= 0.0f)
			return;

		AllocatedBuildings[2] = NewHouse;
		FoundHouseOccupant = occupant;
	}
}

void UBuildingComponent::SetJobHouseEducation(int32 TimeToCompleteDay, TArray<ACitizen*> Roommates)
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	for (int32 i = 0; i < AllocatedBuildings.Num(); i++) {
		ABuilding* building = AllocatedBuildings[i];

		if (!IsValid(building) || building == School || building == Employment || building == House)
			continue;

		if (i == 0 && IsValid(School))
			School->RemoveVisitor(School->GetOccupant(citizen), citizen);
		else if (i == 1 && IsValid(Employment))
			Employment->RemoveCitizen(citizen);
		else if (i == 2 && IsValid(House))
			RemoveCitizenFromHouse(citizen, Roommates);

		if (i == 0) {
			ASchool* school = Cast<ASchool>(building);

			school->AddVisitor(school->GetOccupant(citizen), citizen);
		}
		else if (i == 1) {
			Cast<AWork>(building)->AddCitizen(citizen);
		}
		else {
			if (IsValid(Orphanage))
				Orphanage->RemoveVisitor(Orphanage->GetOccupant(citizen), citizen);

			AHouse* house = Cast<AHouse>(building);

			if (FoundHouseOccupant) {
				house->AddVisitor(FoundHouseOccupant, citizen);

				FoundHouseOccupant = nullptr;
			}
			else {
				house->AddCitizen(citizen);

				for (ACitizen* c : Roommates)
					house->AddVisitor(citizen, c);
			}
		}

		SetAcquiredTime(i, GetWorld()->GetTimeSeconds());
	}
}

TArray<ACitizen*> UBuildingComponent::GetRoomates()
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	TArray<ACitizen*> citizens = citizen->BioComponent->Children;
	citizens.Append(citizen->BioComponent->Siblings);

	TArray<ACitizen*> roommates;

	for (ACitizen* c : citizens) {
		if (IsValid(House) && House->GetOccupant(citizen) == citizen && House->GetVisitors(citizen).Contains(c)) {
			roommates.Add(c);

			continue;
		}

		if (c->BioComponent->Age >= 18 || IsValid(c->BuildingComponent->House))
			continue;

		int32 likeness = 0;

		c->Camera->CitizenManager->PersonalityComparison(citizen, c, likeness);

		if (likeness < 0)
			continue;

		roommates.Add(c);
	}

	if (citizen->BioComponent->Partner != nullptr)
		roommates.Add(citizen->BioComponent->Partner.Get());

	return roommates;
}

float UBuildingComponent::GetAcquiredTime(int32 Index)
{
	return TimeOfAcquirement[Index];
}

void UBuildingComponent::SetAcquiredTime(int32 Index, float Time)
{
	TimeOfAcquirement[Index] = Time;
}

bool UBuildingComponent::CanFindAnything(int32 TimeToCompleteDay, FFactionStruct* Faction)
{
	if (Faction == nullptr || Faction->Police.Arrested.Contains(Cast<ACitizen>(GetOwner())))
		return false;

	for (int32 i = 0; i < 3; i++)
		if (!HasRecentlyAcquired(i, TimeToCompleteDay))
			return true;

	return false;
}

bool UBuildingComponent::HasRecentlyAcquired(int32 Index, int32 TimeToCompleteDay)
{
	return GetWorld()->GetTimeSeconds() < GetAcquiredTime(Index) + TimeToCompleteDay;
}

void UBuildingComponent::RemoveOnReachingWorkAge(FFactionStruct* Faction)
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	if (citizen->BioComponent->Age < citizen->Camera->PoliticsManager->GetLawValue(Faction->Name, "Work Age")) {
		citizen->Camera->TimerManager->RemoveTimer("Orphanage", citizen);

		if (citizen->BioComponent->Age < citizen->Camera->PoliticsManager->GetLawValue(Faction->Name, "Education Age"))
			School->RemoveVisitor(School->GetOccupant(citizen), citizen);

		return;
	}

	if (IsValid(Orphanage) && !citizen->Camera->TimerManager->DoesTimerExist("Orphanage", citizen)) {
		int32 timeToCompleteDay = citizen->Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();

		TArray<FTimerParameterStruct> params;
		citizen->Camera->TimerManager->SetParameter(citizen, params);
		citizen->Camera->TimerManager->CreateTimer("Orphanage", citizen, timeToCompleteDay * 2.0f, "Kickout", params, false);
	}
	
	if (IsValid(School))
		School->RemoveVisitor(School->GetOccupant(citizen), citizen);
}

//
// Work
//
bool UBuildingComponent::CanWork(ABuilding* WorkBuilding)
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	FFactionStruct* faction = citizen->Camera->ConquestManager->GetFaction("", citizen);

	if (citizen->BioComponent->Age < citizen->Camera->PoliticsManager->GetLawValue(faction->Name, "Work Age"))
		return false;

	if (WorkBuilding->IsA<ABooster>() && !Cast<ABooster>(WorkBuilding)->DoesPromoteFavouringValues(citizen))
		return false;

	return true;
}

bool UBuildingComponent::WillWork()
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	FFactionStruct* faction = citizen->Camera->ConquestManager->GetFaction("", citizen);

	int32 pensionAge = citizen->Camera->PoliticsManager->GetLawValue(faction->Name, "Pension Age");

	if (citizen->BioComponent->Age < pensionAge)
		return true;

	int32 pension = citizen->Camera->PoliticsManager->GetLawValue(faction->Name, "Pension");

	if (IsValid(House) && pension >= House->GetAmount(citizen))
		return false;

	return true;
}

//
// Family
//
void UBuildingComponent::RemoveFromHouse()
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	if (!IsValid(House) || House->GetOccupant(citizen) == citizen)
		return;

	TArray<ACitizen*> likedFamily = citizen->BioComponent->GetLikedFamily(false);

	if (likedFamily.Contains(citizen->BioComponent->Mother.Get()) || likedFamily.Contains(citizen->BioComponent->Father.Get()))
		return;

	FFactionStruct* faction = citizen->Camera->ConquestManager->GetFaction("", citizen);

	if (citizen->BioComponent->Age < citizen->Camera->PoliticsManager->GetLawValue(faction->Name, "Work Age")) {
		for (ABuilding* building : faction->Buildings) {
			if (!building->IsA<AOrphanage>())
				continue;

			for (ACitizen* carer : building->GetOccupied()) {
				if (building->GetVisitors(carer).Num() == building->Space)
					continue;

				House->RemoveVisitor(House->GetOccupant(carer), citizen);

				citizen->BioComponent->Disown();

				Cast<AOrphanage>(building)->AddVisitor(carer, citizen);

				return;
			}
		}
	}
	else {
		House->RemoveVisitor(House->GetOccupant(citizen), citizen);
	}
}

void UBuildingComponent::SelectPreferredPartnersHouse(ACitizen* Citizen, ACitizen* Partner)
{
	if (!IsValid(House) && !IsValid(Partner->BuildingComponent->House))
		return;

	bool bThisHouse = true;

	if (IsValid(House) && IsValid(Partner->BuildingComponent->House)) {
		if (House->IsAVisitor(Citizen)) {
			bThisHouse = false;
		}
		else if (!Partner->BuildingComponent->House->IsAVisitor(Partner)) {
			AHouse* partnersHouse = Partner->BuildingComponent->House;

			int32 h1 = House->GetSatisfactionLevel(House->GetAmount(Citizen)) / 10 + House->Space + House->BaseRent;
			int32 h2 = partnersHouse->GetSatisfactionLevel(partnersHouse->GetAmount(Partner)) / 10 + partnersHouse->Space + partnersHouse->BaseRent;

			if (h2 > h1)
				bThisHouse = false;
		}
	}
	else if (IsValid(Partner->BuildingComponent->House)) {
		bThisHouse = false;
	}

	if ((bThisHouse && House->IsAVisitor(Citizen)) || (!bThisHouse && Partner->BuildingComponent->House->IsAVisitor(Partner)))
		return;

	if (bThisHouse) {
		if (IsValid(Partner->BuildingComponent->House))
			Partner->BuildingComponent->House->RemoveCitizen(Partner);
		else if (IsValid(Partner->BuildingComponent->Orphanage))
			Partner->BuildingComponent->Orphanage->RemoveVisitor(Partner->BuildingComponent->Orphanage->GetOccupant(Partner), Partner);

		House->AddVisitor(Citizen, Partner);
	}
	else {
		if (IsValid(House))
			House->RemoveCitizen(Citizen);
		else if (IsValid(Orphanage))
			Orphanage->RemoveVisitor(Orphanage->GetOccupant(Citizen), Citizen);

		Partner->BuildingComponent->House->AddVisitor(Partner, Citizen);
	}

	SetAcquiredTime(2, GetWorld()->GetTimeSeconds());
	Partner->BuildingComponent->SetAcquiredTime(2, GetWorld()->GetTimeSeconds());
}

//
// House
//
void UBuildingComponent::RemoveCitizenFromHouse(ACitizen* Citizen, TArray<ACitizen*> Roommates)
{
	if (House->GetOccupied().Contains(Citizen)) {
		TArray<ACitizen*> leftoverCitizens;

		for (ACitizen* c : House->GetVisitors(Citizen))
			if (!Roommates.Contains(c))
				leftoverCitizens.Add(c);

		House->RemoveCitizen(Citizen);

		if (!leftoverCitizens.IsEmpty()) {
			int32 index = Citizen->Camera->Stream.RandRange(0, leftoverCitizens.Num() - 1);

			ACitizen* newOccupant = leftoverCitizens[index];
			leftoverCitizens.RemoveAt(index);

			House->AddCitizen(newOccupant);

			for (ACitizen* c : leftoverCitizens)
				House->AddVisitor(newOccupant, c);
		}
	}
	else
		House->RemoveVisitor(House->GetOccupant(Citizen), Citizen);
}