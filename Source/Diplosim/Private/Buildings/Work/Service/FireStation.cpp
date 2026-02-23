#include "Buildings/Work/Service/FireStation.h"

#include "AI/DiplosimAIController.h"
#include "AI/Citizen/Citizen.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/DiplosimTimerManager.h"

AFireStation::AFireStation()
{

}

void AFireStation::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (GetOccupied().Contains(Citizen))
		Production(Citizen);
}

bool AFireStation::IsAtWork(ACitizen* Citizen)
{
	bool bWorking = Super::IsAtWork(Citizen);

	if (!bWorking) {
		FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);
		
		for (auto& element : faction->BuildingsOnFire) {
			if (element.Value != Citizen)
				continue;

			bWorking = true;

			break;
		}
	}

	return bWorking;
}

void AFireStation::Production(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);
	TArray<ACitizen*> citizens;

	if (IsValid(Citizen))
		citizens.Add(Citizen);
	else
		citizens = GetCitizensAtBuilding();

	for (auto& element : faction->BuildingsOnFire) {
		if (citizens.IsEmpty())
			return;

		if (IsValid(element.Value))
			continue;

		citizens[0]->AIController->AIMoveTo(element.Key);

		element.Value = citizens[0];
		citizens.RemoveAt(0);
	}
}

void AFireStation::PutOutFire(ABuilding* Building, ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	if (!faction->BuildingsOnFire.Contains(Building))
		return;

	faction->BuildingsOnFire.Remove(Building);

	Camera->Grid->AtmosphereComponent->ClearFire(Building);

	// Niagara system for 2 seconds to indicate putting out fire (mini rain cloud?)

	Camera->TimerManager->CreateTimer("RespondedToFire", Citizen, 2.0f, "DefaultAction", {}, false, true);
}