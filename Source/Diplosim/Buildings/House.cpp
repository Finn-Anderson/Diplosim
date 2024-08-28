#include "House.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"

AHouse::AHouse()
{
}

void AHouse::UpkeepCost()
{
	int32 rent = 0;

	for (FRentStruct rentStruct : Camera->CitizenManager->RentList) {
		if (IsA(rentStruct.HouseType)) {
			rent = rentStruct.Rent;

			break;
		}
	}

	for (ACitizen* citizen : GetOccupied()) {
		if (citizen->Balance < rent) {
			RemoveCitizen(citizen);
		}
		else {
			citizen->Balance -= rent;
		}
	}

	int32 total = rent * Occupied.Num();
	Camera->ResourceManager->AddUniversalResource(Money, total);
}

void AHouse::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	Citizen->bGain = true;
}

void AHouse::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	Citizen->bGain = false;
}

bool AHouse::AddCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.House = this;

	return true;
}

bool AHouse::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.House = nullptr;

	return true;
}