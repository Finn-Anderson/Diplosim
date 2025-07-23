#include "Buildings/Work/Booster.h"

#include "Kismet/GameplayStatics.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Map/Grid.h"
#include "AI/Citizen.h"

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
	TArray<TEnumAsByte<EObjectTypeQuery>> objects;

	TArray<AActor*> ignore;
	ignore.Add(Camera->Grid);

	for (FResourceHISMStruct resourceStruct : Camera->Grid->MineralStruct)
		ignore.Add(resourceStruct.Resource);

	for (FResourceHISMStruct resourceStruct : Camera->Grid->FlowerStruct)
		ignore.Add(resourceStruct.Resource);

	for (FResourceHISMStruct resourceStruct : Camera->Grid->TreeStruct)
		ignore.Add(resourceStruct.Resource);

	ignore.Append(Camera->CitizenManager->Citizens);
	ignore.Append(Camera->CitizenManager->Enemies);

	TArray<AActor*> actors;

	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), GetActorLocation(), Range, objects, nullptr, ignore, actors);

	TArray<ABuilding*> buildings;

	for (AActor* actor : actors) {
		if (!BuildingsToBoost.Contains(actor->GetClass()))
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

		bool bContainsParty = Camera->CitizenManager->Parties.Contains(party);

		if ((!Camera->CitizenManager->Religions.Contains(religion) && !bContainsParty) || element.Value == Citizen->Spirituality.Faith || (bContainsParty && element.Value == Camera->CitizenManager->GetMembersParty(Citizen)->Party))
			continue;

		bFavouring = false;

		break;
	}

	return bFavouring;
}
