#include "House.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "AI/BuildingComponent.h"
#include "AI/BioComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"

AHouse::AHouse()
{
	BaseRent = 0;
}

void AHouse::BeginPlay()
{
	Super::BeginPlay();

	RentStruct.Empty();

	for (int32 i = 0; i < MaxCapacity; i++) {
		FRentStruct rent;
		rent.Rent = BaseRent;

		RentStruct.Add(rent);
	}
}

int32 AHouse::GetSatisfactionLevel(int32 Rent)
{
	int32 difference = BaseRent - Rent;
	int32 percentage = difference * (50.0f / (BaseRent / 2.0f)) + 50;

	return FMath::Clamp(percentage, 0, 100);
}

void AHouse::CollectRent(FFactionStruct* Faction, ACitizen* Citizen)
{
	TArray<ACitizen*> family;
	int32 total = Citizen->Balance;
	int32 rent = GetRent(Citizen);

	FOccupantStruct occupant;
	occupant.Occupant = Citizen;

	int32 index = Occupied.Find(occupant);

	for (ACitizen* c : Occupied[index].Visitors) {
		family.Add(c);

		total += c->Balance;
	}

	if (total < rent) {
		for (ACitizen* c : Citizen->BioComponent->GetLikedFamily(false)) {
			if (!IsValid(c->BuildingComponent->House) || c->Balance - c->BuildingComponent->House->GetRent(c) - rent <= 0 || family.Contains(c))
				continue;

			family.Add(c);

			total += c->Balance;
		}
	}

	if (total < rent) {
		RemoveCitizen(Citizen);
	}
	else {
		int32 pay = FMath::Min(rent, Citizen->Balance);
		Citizen->Balance -= pay;
		rent -= pay;

		if (rent > 0) {
			for (int32 i = 0; i < rent; i++) {
				index = Camera->Stream.RandRange(0, family.Num() - 1);

				family[index]->Balance -= 1;

				if (family[index]->Balance == 0)
					family.RemoveAt(index);
			}
		}

		Camera->ResourceManager->AddUniversalResource(Faction, Camera->ResourceManager->Money, rent);
	}
}

void AHouse::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	Citizen->bGain = true;

	Camera->TimerManager->UpdateTimerLength("Energy", this, 1);
}

void AHouse::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	Citizen->bGain = false;

	int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();
	Camera->TimerManager->UpdateTimerLength("Energy", this, (timeToCompleteDay / 100) * Citizen->EnergyMultiplier);
}

bool AHouse::AddCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck)
		return false;

	SetRent(Citizen);
	Citizen->BuildingComponent->House = this;

	Citizen->AIController->DefaultAction();

	return true;
}

bool AHouse::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	RemoveRent(Citizen);
	Citizen->BuildingComponent->House = nullptr;

	Citizen->AIController->DefaultAction();

	return true;
}

void AHouse::AddVisitor(ACitizen* Occupant, ACitizen* Visitor)
{
	Visitor->BuildingComponent->House = this;

	Super::AddVisitor(Occupant, Visitor);
}

void AHouse::RemoveVisitor(ACitizen* Occupant, ACitizen* Visitor)
{
	Visitor->BuildingComponent->House = nullptr;

	Super::RemoveVisitor(Occupant, Visitor);
}

FRentStruct* AHouse::GetBestAvailableRoom()
{
	FRentStruct* rent = nullptr;

	for (FRentStruct& r : RentStruct) {
		if (IsValid(r.Occupant))
			continue;

		if (rent == nullptr) {
			rent = &r;

			continue;
		}

		if (rent->Rent > r.Rent)
			rent = &r;
	}

	return rent;
}

void AHouse::SetRent(ACitizen* Citizen)
{
	FRentStruct* rent = GetBestAvailableRoom();
	rent->Occupant = Citizen;
}

void AHouse::RemoveRent(ACitizen* Citizen)
{
	FRentStruct rent;
	rent.Occupant = Citizen;

	int32 index = RentStruct.Find(rent);

	RentStruct[index].Occupant = nullptr;
}

int32 AHouse::GetRent(ACitizen* Citizen)
{
	ACitizen* occupant = GetOccupant(Citizen);

	FRentStruct rent;
	rent.Occupant = occupant;

	int32 index = RentStruct.Find(rent);

	return RentStruct[index].Rent;
}

void AHouse::UpdateRent(int32 Index, int32 NewRent)
{
	if (Index == INDEX_NONE) {
		for (FRentStruct& rent : RentStruct)
			rent.Rent = NewRent;
	}
	else {
		RentStruct[Index].Rent = NewRent;
	}
}