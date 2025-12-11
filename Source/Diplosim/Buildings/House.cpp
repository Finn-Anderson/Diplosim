#include "House.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"

AHouse::AHouse()
{
	Rent = 0;
	BaseRent = Rent;
}

int32 AHouse::GetSatisfactionLevel()
{
	int32 difference = BaseRent - Rent;
	int32 percentage = difference * (50.0f / (BaseRent / 2.0f)) + 50;

	return FMath::Clamp(percentage, 0, 100);
}

void AHouse::GetRent(FFactionStruct* Faction, ACitizen* Citizen)
{
	TArray<ACitizen*> family;
	int32 total = Citizen->Balance;

	FOccupantStruct occupant;
	occupant.Occupant = Citizen;

	int32 index = Occupied.Find(occupant);

	for (ACitizen* c : Occupied[index].Visitors) {
		family.Add(c);

		total += c->Balance;
	}

	if (total < Rent) {
		for (ACitizen* c : Citizen->GetLikedFamily(false)) {
			if (!IsValid(c->Building.House) || c->Balance - c->Building.House->Rent - Rent <= 0 || family.Contains(c))
				continue;

			family.Add(c);

			total += c->Balance;
		}
	}

	if (total < Rent) {
		RemoveCitizen(Citizen);
	}
	else {
		for (int32 i = 0; i < Rent; i++) {
			if (Citizen->Balance == 0) {
				index = Camera->Grid->Stream.RandRange(0, family.Num() - 1);

				family[index]->Balance -= 1;

				if (family[index]->Balance == 0)
					family.RemoveAt(index);
			}
			else {
				Citizen->Balance -= 1;
			}
		}

		Camera->ResourceManager->AddUniversalResource(Faction, Camera->ResourceManager->Money, Rent);
	}
}

void AHouse::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	Citizen->bGain = true;

	Camera->CitizenManager->UpdateTimerLength("Energy", this, 1);
}

void AHouse::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	Citizen->bGain = false;

	int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();
	Camera->CitizenManager->UpdateTimerLength("Energy", this, (timeToCompleteDay / 100) * Citizen->EnergyMultiplier);
}

bool AHouse::AddCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.House = this;

	Citizen->AIController->DefaultAction();

	return true;
}

bool AHouse::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.House = nullptr;

	Citizen->AIController->DefaultAction();

	return true;
}

void AHouse::AddVisitor(ACitizen* Occupant, ACitizen* Visitor)
{
	Visitor->Building.House = this;

	Super::AddVisitor(Occupant, Visitor);
}

void AHouse::RemoveVisitor(ACitizen* Occupant, ACitizen* Visitor)
{
	Visitor->Building.House = nullptr;

	Super::RemoveVisitor(Occupant, Visitor);
}