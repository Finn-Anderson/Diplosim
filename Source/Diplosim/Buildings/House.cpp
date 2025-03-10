#include "House.h"

#include "Kismet/GameplayStatics.h"

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

	QualityCap = 70;
}

int32 AHouse::GetQuality()
{
	return FMath::Clamp(Rent * 12, 0, QualityCap);
}

void AHouse::UpkeepCost()
{
	for (ACitizen* citizen : GetOccupied()) {
		if (citizen->Balance < Rent) {
			RemoveCitizen(citizen);
		}
		else {
			citizen->Balance -= Rent;
		}
	}

	int32 total = Rent * Occupied.Num();
	Camera->ResourceManager->AddUniversalResource(Money, total);
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

	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);
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