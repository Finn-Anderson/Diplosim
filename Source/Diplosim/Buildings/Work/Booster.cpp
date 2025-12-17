#include "Buildings/Work/Booster.h"

#include "Kismet/KismetSystemLibrary.h"

#include "AI/Citizen.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Universal/DiplosimGameModeBase.h"

ABooster::ABooster()
{
	Range = 100.0f;

	bHolyPlace = false;
}

void ABooster::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (bHolyPlace)
		Citizen->bWorshipping = true;
}

void ABooster::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (bHolyPlace)
		Citizen->bWorshipping = false;
}

void ABooster::SetBroadcastType(FString Type)
{
	for (auto& element : BuildingsToBoost)
		element.Value = Type;

	for (int32 i = GetOccupied().Num() - 1; i > -1; i--)
		if (!GetOccupied()[i]->CanWork(this))
			RemoveCitizen(GetOccupied()[i]);
}

TArray<ABuilding*> ABooster::GetAffectedBuildings()
{
	FOverlapsStruct overlaps;
	overlaps.bBuildings = true;

	TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, this, Range, overlaps, EFactionType::Same);

	TArray<ABuilding*> buildings;

	for (AActor* actor : actors) {
		TArray<TSubclassOf<ABuilding>> buildingClasses;
		BuildingsToBoost.GenerateKeyArray(buildingClasses);

		bool bContainsBuilding = false;

		for (TSubclassOf<ABuilding> buildingClass : buildingClasses) {
			if (actor->FindNearestCommonBaseClass(buildingClasses[0]) != buildingClass)
				continue;

			bContainsBuilding = true;

			break;
		}

		if (!bContainsBuilding)
			continue;

		buildings.Add(Cast<ABuilding>(actor));
	}

	return buildings;
}

bool ABooster::DoesPromoteFavouringValues(ACitizen* Citizen)
{
	bool bFavouring = true;

	for (auto& element : BuildingsToBoost) {
		FReligionStruct religion;
		religion.Faith = element.Value;

		FPartyStruct party;
		party.Party = element.Value;

		FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

		bool bContainsParty = faction->Politics.Parties.Contains(party);

		if ((!Camera->CitizenManager->Religions.Contains(religion) && !bContainsParty) || element.Value == Citizen->Spirituality.Faith || (bContainsParty && element.Value == Camera->PoliticsManager->GetMembersParty(Citizen)->Party))
			continue;

		bFavouring = false;

		break;
	}

	return bFavouring;
}