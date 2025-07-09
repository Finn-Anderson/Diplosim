#include "Player/Managers/ConquestManager.h"

#include "Components/WidgetComponent.h"

#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Map/Grid.h"
#include "AI/Citizen.h"
#include "AI/AttackComponent.h"
#include "AI/DiplosimAIController.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Misc/Portal.h"

UConquestManager::UConquestManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	EmpireName = "";
	WorldSize = 100;
	PercentageIsland = 10;
	EnemiesNum = 2;

	playerCapitalIndex = 0;
}

void UConquestManager::BeginPlay()
{
	Super::BeginPlay();
	
	Camera = Cast<ACamera>(GetOwner());
}

void UConquestManager::GenerateWorld()
{
	World.Empty();
	Factions.Empty();
	
	TArray<FWorldTileStruct*> choosableWorldTiles;

	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)WorldSize));
	
	for (int32 x = 0; x < bound; x++) {
		for (int32 y = 0; y < bound; y++) {
			FWorldTileStruct tile;
			tile.X = x;
			tile.Y = y;

			World.Add(tile);
		}
	}

	for (FWorldTileStruct& tile : World)
		choosableWorldTiles.Add(&tile);

	if (EmpireName == "")
		EmpireName = Camera->ColonyName + " Empire";

	TArray<FString> factions;
	factions.Add(EmpireName);
	
	FString empireNames;
	FFileHelper::LoadFileToString(empireNames, *(FPaths::ProjectDir() + "/Content/Custom/Colony/EmpireNames.txt"));

	TArray<FString> empireParsed;
	empireNames.ParseIntoArray(empireParsed, TEXT(","));

	FString colonyNames;
	FFileHelper::LoadFileToString(colonyNames, *(FPaths::ProjectDir() + "/Content/Custom/Colony/ColonyNames.txt"));

	TArray<FString> colonyParsed;
	colonyNames.ParseIntoArray(colonyParsed, TEXT(","));

	for (int32 i = 0; i < EnemiesNum; i++) {
		int32 index = Camera->Grid->Stream.RandRange(0, empireParsed.Num() - 1);
		factions.Add(empireParsed[index]);

		if (empireParsed[index] != "")
			empireParsed.RemoveAt(index);
	}

	int32 islandsNum = FMath::RoundHalfFromZero(choosableWorldTiles.Num() * (PercentageIsland / 100.0f));

	TArray<FFactionStruct> factionIcons = DefaultOccupierTextureList;

	for (FString name : factions) {
		if (islandsNum == 0)
			return;

		int32 index = Camera->Grid->Stream.RandRange(0, choosableWorldTiles.Num() - 1);
		FWorldTileStruct* tile = choosableWorldTiles[index];

		tile->bIsland = true;
		tile->bCapital = true;

		FFactionStruct f;
		f.Name = name;

		tile->Owner = name;

		int32 tIndex = Camera->Grid->Stream.RandRange(0, factionIcons.Num() - 1);

		f.Texture = factionIcons[tIndex].Texture;
		f.Colour = factionIcons[tIndex].Colour;

		factionIcons.RemoveAt(tIndex);

		int32 rIndex = Camera->Grid->Stream.RandRange(0, ResourceList.Num() - 1);
		tile->Resource = ResourceList[rIndex].ResourceClass;

		tile->Abundance = Camera->Grid->Stream.RandRange(1, 5);

		if (name != EmpireName) {
			int32 cIndex = Camera->Grid->Stream.RandRange(0, colonyParsed.Num() - 1);
			tile->Name = colonyParsed[cIndex];

			if (name == "") {
				f.Name = tile->Name + "Empire";

				tile->Owner = f.Name;
			}

			colonyParsed.RemoveAt(cIndex);
		}
		else {
			tile->Name = Camera->ColonyName;

			playerCapitalIndex = index;
		}

		Factions.Add(f);

		choosableWorldTiles.RemoveAt(index);
		islandsNum--;
	}

	while (islandsNum > 0) {
		int32 index = Camera->Grid->Stream.RandRange(0, choosableWorldTiles.Num() - 1);
		FWorldTileStruct* tile = choosableWorldTiles[index];

		tile->bIsland = true;

		if (!colonyParsed.IsEmpty()) {
			int32 cIndex = Camera->Grid->Stream.RandRange(0, colonyParsed.Num() - 1);
			tile->Name = colonyParsed[cIndex];

			colonyParsed.RemoveAt(cIndex);
		}
		else {
			tile->Name = "Unnamed Island";
		}

		int32 rIndex = Camera->Grid->Stream.RandRange(0, ResourceList.Num() - 1);
		tile->Resource = ResourceList[rIndex].ResourceClass;

		tile->Abundance = Camera->Grid->Stream.RandRange(1, 5);

		choosableWorldTiles.RemoveAt(index);
		islandsNum--;
	}
}

void UConquestManager::StartConquest()
{
	if (Portal->IsHidden()) {
		Portal->Destroy();
		Portal = nullptr;

		return;
	}

	for (FWorldTileStruct& tile : World) {
		if (tile.Owner == "" || tile.Owner == EmpireName)
			continue;

		for (int32 i = 0; i < Camera->Grid->Stream.RandRange(25, 50); i++)
			SpawnCitizenAtColony(tile);
	}
}

void UConquestManager::GiveResource()
{
	if (!IsValid(Portal))
		return;

	TArray<FWorldTileStruct*> occupiedIslands;

	for (FWorldTileStruct& tile : World) {
		if (!tile.bIsland || tile.Owner == "")
			continue;

		occupiedIslands.Add(&tile);

		int32 eventChance = Camera->Grid->Stream.RandRange(0, 100);

		TArray<float> multipliers = { 1.0f, 0.0f, 0.0f };

		if (eventChance == 100)
			multipliers = ProduceEvent();

		int32 value = 0;
		bool bNegative = false;

		if (multipliers[1] != 0.0f) {
			value = Camera->Grid->Stream.FRandRange(0.0f, FMath::Abs(multipliers[1])) * tile.Citizens.Num();

			if (multipliers[1] < 0.0f)
				bNegative = true;
		}
		else if (multipliers[2] != 0.0f) {
			value = Camera->Grid->Stream.RandRange(0, FMath::Abs((int32)multipliers[2]));

			if (multipliers[2] < 0.0f)
				bNegative = true;
		}

		if (value > 0)
			ModifyCitizensEvent(tile, value, bNegative);

		tile.HoursColonised++;

		if (tile.Owner != EmpireName || tile.bCapital)
			continue;

		FIslandResourceStruct islandResource;
		islandResource.ResourceClass = tile.Resource;

		int32 index = ResourceList.Find(islandResource);

		if (multipliers[0] < 1.0f)
			value = Camera->Grid->Stream.FRandRange(multipliers[0], 1.0f);
		else
			value = Camera->Grid->Stream.FRandRange(1.0f, multipliers[0]);

		float amount = Camera->Grid->Stream.RandRange(ResourceList[index].Min, ResourceList[index].Max) * value * tile.Abundance;

		float multiplier = 0.0f;

		for (ACitizen* citizen : tile.Citizens)
			multiplier += citizen->GetProductivity() + (amount * 0.1f);

		Camera->ResourceManager->AddUniversalResource(tile.Resource, amount * (multiplier / tile.Citizens.Num()));
	}

	TArray<FFactionStruct> factionsToRemove;

	for (FFactionStruct& faction : Factions) {
		if (faction.AtWar.IsEmpty() && faction.WarFatigue > 0)
			faction.WarFatigue--;
		else if (!faction.AtWar.IsEmpty())
			faction.WarFatigue++;

		if (faction.Name == EmpireName)
			continue;

		bool bRulesAnIsland = false;

		for (FWorldTileStruct* island : occupiedIslands) {
			if (island->Owner != faction.Name)
				continue;

			bRulesAnIsland = true;

			break;
		}

		if (!bRulesAnIsland) {
			factionsToRemove.Add(faction);

			continue;
		}

		SetFactionCulture(faction);

		SetFactionsHappiness(faction, occupiedIslands);

		Camera->UpdateFactionHappiness();

		EvaluateDiplomacy(faction);

		EvaluateAI(faction, occupiedIslands);
	}

	for (FFactionStruct faction : factionsToRemove)
		Factions.Remove(faction);
}

void UConquestManager::SpawnCitizenAtColony(FWorldTileStruct& Tile)
{
	FActorSpawnParameters params;
	params.bNoFail = true;

	ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(CitizenClass, FVector(0.0f, 0.0f, -10000.0f), FRotator::ZeroRotator, params);

	citizen->SetSex(Tile.Citizens);

	for (int32 j = 0; j < 2; j++)
		citizen->GivePersonalityTrait();

	citizen->BioStruct.Age = 17;
	citizen->Birthday();

	citizen->HealthComponent->AddHealth(100);

	citizen->ApplyResearch();

	citizen->SetActorHiddenInGame(true);

	Tile.Citizens.Add(citizen);
}

void UConquestManager::MoveToColony(FFactionStruct Faction, FWorldTileStruct Tile, ACitizen* Citizen)
{
	if (!CanTravel(Citizen))
		return;

	int32 i = World.Find(Tile);

	FWorldTileStruct* t = &World[i];

	if (t->Owner == "") {
		t->Owner = Faction.Name;

		FString type = "Neutral";
		FFactionStruct& playerFaction = GetFactionFromOwner(EmpireName);

		if (t->Owner == EmpireName || playerFaction.Allies.Contains(t->Owner))
			type = "Good";
		else if (playerFaction.AtWar.Contains(t->Owner))
			type = "Bad";

		Camera->NotifyLog(type, "Founding new colony", t->Name);
	}
	else if (t->Owner != Faction.Name && !Faction.Allies.Contains(t->Owner)) {
		if (t->RaidStarterName == "") {
			t->RaidStarterName = Faction.Name;

			Camera->SetIslandBeingRaided(*t, true);

			if (t->Owner == EmpireName)
				DisplayConquestNotification(Faction.Name + " is sending a raid party to " + t->Name, Faction.Name, false);
		}

		FRaidStruct raid;
		raid.Owner = Faction.Name;

		int32 index = t->RaidParties.Find(raid);

		if (index == INDEX_NONE)
			t->RaidParties.Add(raid);
	}
	
	FWorldTileStruct* tile = GetColonyContainingCitizen(Citizen);
	tile->Moving.Add(Citizen, World.Find(*tile));

	if (tile->bCapital && tile->Owner == EmpireName)
		Citizen->AIController->AIMoveTo(Portal);
	else
		Camera->CitizenManager->CreateTimer("Transmission", Citizen, Camera->Grid->Stream.RandRange(10, 40), FTimerDelegate::CreateUObject(this, &UConquestManager::StartTransmissionTimer, Citizen), false);

	Camera->UpdateMoveCitizen(Citizen, *tile, *t);
}

void UConquestManager::StartTransmissionTimer(ACitizen* Citizen)
{
	FWorldTileStruct* oldTile = GetColonyContainingCitizen(Citizen);

	int32 index = *oldTile->Moving.Find(Citizen);

	FWorldTileStruct* targetTile = &World[index];

	FFactionStruct& faction = GetFactionFromOwner(oldTile->Owner);

	FRaidStruct raid;
	raid.Owner = faction.Name;

	if (!CanTravel(Citizen) || (!faction.Allies.Contains(targetTile->Owner) && !targetTile->RaidParties.Contains(raid))) {
		oldTile->Moving.Remove(Citizen);

		if (oldTile->bCapital && oldTile->Owner == EmpireName)
			Citizen->AIController->DefaultAction();

		return;
	}

	oldTile->Citizens.Remove(Citizen);

	int32 x = FMath::Abs(oldTile->X - targetTile->X);
	int32 y = FMath::Abs(oldTile->Y - targetTile->Y);

	int32 time = (x + y) * 10;

	Camera->CitizenManager->CreateTimer("Travel", Citizen, time, FTimerDelegate::CreateUObject(this, &UConquestManager::AddCitizenToColony, oldTile, targetTile, Citizen), false);

	if (oldTile->bCapital && oldTile->Owner == EmpireName)
		Citizen->ColonyIslandSetup();

	Camera->DisableMoveBtn(Citizen);
}

void UConquestManager::AddCitizenToColony(FWorldTileStruct* OldTile, FWorldTileStruct* Tile, ACitizen* Citizen)
{
	if (Tile->Owner != OldTile->Owner) {
		FFactionStruct& faction = GetFactionFromOwner(OldTile->Owner);

		FRaidStruct raid;
		raid.Owner = faction.Name;

		if (!faction.Allies.Contains(Tile->Owner) && !Tile->RaidParties.Contains(raid)) {
			MoveToColony(faction, *OldTile, Citizen);

			return;
		}
		else if (faction.Allies.Contains(Tile->Owner)) {
			if (Tile->Stationed.Contains(faction.Name)) {
				FStationedStruct* stationed = Tile->Stationed.Find(faction.Name);
				stationed->Guards.Add(Citizen);
			}
			else {
				FStationedStruct stationed;
				stationed.Guards.Add(Citizen);

				Tile->Stationed.Add(faction.Name, stationed);
			}
		}
		else {
			int32 index = World.Find(*OldTile);

			int32 raidIndex = Tile->RaidParties.Find(raid);

			Tile->RaidParties[raidIndex].Raiders.Add(Citizen, index);

			if (CanStartRaid(Tile, &GetFactionFromOwner(OldTile->Owner))) {
				FString type = "Neutral";
				FFactionStruct& playerFaction = GetFactionFromOwner(EmpireName);

				if (Tile->Owner == EmpireName || playerFaction.Allies.Contains(Tile->Owner))
					type = "Bad";
				else if (playerFaction.AtWar.Contains(Tile->Owner))
					type = "Good";

				Camera->NotifyLog(type, OldTile->Owner + " is raiding " + Tile->Owner, Tile->Name);

				if (Tile->bCapital && Tile->Owner == EmpireName) {
					for (auto& element : Tile->RaidParties[raidIndex].Raiders) {
						element.Key->MainIslandSetup();

						Camera->CitizenManager->SetupRebel(element.Key);
					}
				}
				else
					Camera->CitizenManager->CreateTimer("ColonyRaid" + Tile->Name, GetOwner(), 1.0f, FTimerDelegate::CreateUObject(this, &UConquestManager::EvaluateRaid, Tile), true);
			}
		}
	}
	else if (Tile->bCapital && Tile->Owner == EmpireName) {
		Citizen->MainIslandSetup();

		Citizen->SetActorLocation(Portal->GetActorLocation());
	}
	else {
		Tile->Citizens.Add(Citizen);
	}

	OldTile->Moving.Remove(Citizen);

	Camera->CitizenManager->CreateTimer("RecentlyMoved", Citizen, 300, FTimerDelegate::CreateUObject(this, &UConquestManager::RemoveFromRecentlyMoved, Citizen), false);

	Camera->UpdateMoveCitizen(Citizen, *OldTile, *Tile);
}

TArray<float> UConquestManager::ProduceEvent()
{
	int32 index = Camera->Grid->Stream.RandRange(0, EventList.Num() - 1);

	return { EventList[index].ResourceMultiplier, EventList[index].CitizenMultiplier, EventList[index].Citizens };
}

FWorldTileStruct* UConquestManager::GetColonyContainingCitizen(ACitizen* Citizen)
{
	for (FWorldTileStruct& tile : World) {
		bool bStationed = false;

		for (auto& element: tile.Stationed) {
			if (!element.Value.Guards.Contains(Citizen))
				continue;
			
			bStationed = true;

			break;
		}

		if (!tile.bIsland || (!tile.Citizens.Contains(Citizen) && !bStationed))
			continue;

		return &tile;
	}

	return nullptr;
}

void UConquestManager::ModifyCitizensEvent(FWorldTileStruct& Tile, int32 Amount, bool bNegative)
{
	for (int32 i = 0; i < Amount; i++) {
		if (bNegative) {
			int32 index = Camera->Grid->Stream.RandRange(0, Tile.Citizens.Num() - 1);

			Tile.Citizens[index]->HealthComponent->TakeHealth(1000, Tile.Citizens[index]);
		}
		else {
			SpawnCitizenAtColony(Tile);
		}
	}
}

bool UConquestManager::CanTravel(class ACitizen* Citizen)
{
	if (RecentlyMoved.Contains(Citizen) || Camera->CitizenManager->Injured.Contains(Citizen) || Camera->CitizenManager->Infected.Contains(Citizen) || Citizen->BioStruct.Age < Camera->CitizenManager->GetLawValue("Work Age") || !Citizen->WillWork())
		return false;

	return true;
}

FWorldTileStruct UConquestManager::GetTileInformation(int32 Index)
{
	FWorldTileStruct tile = World[Index];

	if (tile.bCapital && tile.Owner == EmpireName)
		tile.Citizens = Camera->CitizenManager->Citizens;

	return tile;
}

FFactionStruct& UConquestManager::GetFactionFromOwner(FString Owner)
{
	for (FFactionStruct& faction : Factions) {
		if (faction.Name != Owner)
			continue;

		return faction;
	}

	throw std::runtime_error("Faction not found");
}

void UConquestManager::SetFactionTexture(FString Owner, UTexture2D* Texture, FLinearColor Colour)
{
	FFactionStruct& faction = GetFactionFromOwner(Owner);

	faction.Texture = Texture;
	faction.Colour = Colour;

	Camera->UpdateFactionImage();
}

void UConquestManager::SetColonyName(int32 X, int32 Y, FString NewColonyName)
{
	if (NewColonyName == Camera->ColonyName)
		return;
	
	if (NewColonyName == "")
		NewColonyName = "Eggerton";
	
	FWorldTileStruct t;
	t.X = X;
	t.Y = Y;

	int32 index = World.Find(t);

	FWorldTileStruct* tile = &World[index];

	if (tile->bCapital && EmpireName == Camera->ColonyName + " Empire")
		SetTerritoryName(EmpireName, NewColonyName + " Empire");

	tile->Name = NewColonyName;

	if (tile->bCapital && tile->Owner == EmpireName)
		Camera->ColonyName = NewColonyName;
}

void UConquestManager::SetTerritoryName(FString OldEmpireName, FString NewEmpireName)
{
	if (NewEmpireName == OldEmpireName)
		return;
	
	if (NewEmpireName == "")
		NewEmpireName = Camera->ColonyName + " Empire";
	
	for (FFactionStruct& factions : Factions) {
		if (factions.Name != NewEmpireName)
			continue;

		Camera->ShowWarning("Empire name already exists");

		break;
	}

	FString empireNameCheck = NewEmpireName;
	FString checkIfNameValid = empireNameCheck.Replace(*FString(" "), *FString(""));

	if (checkIfNameValid == "") {
		Camera->ShowWarning("Invalid empire name");

		return;
	}

	FFactionStruct faction;
	faction.Name = OldEmpireName;

	int32 index = Factions.Find(faction);
	Factions[index].Name = NewEmpireName;

	for (FWorldTileStruct& tile : World) {
		if (!tile.bIsland || tile.Owner != OldEmpireName)
			continue;

		tile.Owner = NewEmpireName;
	}

	EmpireName = NewEmpireName;

	Camera->UpdateIconEmpireName(OldEmpireName);
}

TArray<ACitizen*> UConquestManager::GetIslandCitizens(FWorldTileStruct Tile)
{
	if (Tile.bCapital && Tile.Owner == EmpireName)
		return Camera->CitizenManager->Citizens;
	else
		return Tile.Citizens;
}

void UConquestManager::RemoveFromRecentlyMoved(ACitizen* Citizen)
{
	RecentlyMoved.Remove(Citizen);
}

void UConquestManager::CancelMovement(ACitizen* Citizen)
{
	FWorldTileStruct* tile = GetColonyContainingCitizen(Citizen);

	if (!tile->Moving.Contains(Citizen))
		return;

	int32 index = *tile->Moving.Find(Citizen);
	FWorldTileStruct* target = &World[index];

	tile->Moving.Remove(Citizen);
	
	if (target->RaidStarterName == EmpireName && CanStartRaid(target, &GetFactionFromOwner(EmpireName))) {
		bool bEmptyRaid = true;

		for (FRaidStruct raid : target->RaidParties) {
			if (raid.Raiders.IsEmpty())
				continue;

			bEmptyRaid = false;

			break;
		}

		if (bEmptyRaid) {
			target->RaidStarterName = "";
			target->RaidParties.Empty();
		}
	}

	Camera->CitizenManager->RemoveTimer("Transmission", Citizen);
}

bool UConquestManager::CanCancelMovement(ACitizen* Citizen)
{
	if (Camera->CitizenManager->FindTimer("Travel", Citizen) != nullptr)
		return false;

	return true;
}

FFactionStruct& UConquestManager::GetCitizenFaction(ACitizen* Citizen)
{
	for (FWorldTileStruct& tile : World) {
		if (!tile.Citizens.Contains(Citizen) || !tile.Moving.Contains(Citizen))
			continue;

		FFactionStruct& faction = GetFactionFromOwner(tile.Owner);

		return faction;
	}

	throw std::runtime_error("Faction not found");
}

FWorldTileStruct* UConquestManager::FindCapital(FFactionStruct& Faction, TArray<FWorldTileStruct*> OccupiedIslands)
{
	for (FWorldTileStruct* island : OccupiedIslands) {
		if (!island->bCapital || island->Owner != Faction.Name)
			continue;

		return island;
	}

	return nullptr;
}

bool UConquestManager::IsCitizenMoving(class ACitizen* Citizen)
{
	for (FWorldTileStruct& tile : World) {
		if (!tile.Moving.Contains(Citizen))
			continue;

		return true;
	}

	return false;
}

//
// Diplomacy
//
void UConquestManager::SetFactionCulture(FFactionStruct& Faction)
{
	if (Faction.Name == EmpireName) {
		TMap<FString, int32> religionCount;

		for (ACitizen* citizen : Camera->CitizenManager->Citizens) {
			FString faith = citizen->Spirituality.Faith;

			if (religionCount.Contains(faith)) {
				int32* count = religionCount.Find(faith);
				count++;
			}
			else {
				religionCount.Add(faith, 1);
			}
		}

		FPartyStruct biggestParty;

		for (FPartyStruct party : Camera->CitizenManager->Parties) {
			if (biggestParty.Members.Num() >= party.Members.Num())
				continue;

			biggestParty = party;
		}

		TTuple<FString, int32> biggestReligion;

		for (auto& element : religionCount) {
			if (biggestReligion.Value >= element.Value)
				continue;

			biggestReligion = element;
		}

		Faction.PartyInPower = biggestParty.Party;
		Faction.Religion = biggestReligion.Key;
	}
	else if (Faction.PartyInPower == "Undecided" || Camera->Grid->Stream.RandRange(0, 480) == 480) {
		int32 partyIndex = Camera->Grid->Stream.RandRange(0, Camera->CitizenManager->Parties.Num() - 1);
		int32 religionIndex = Camera->Grid->Stream.RandRange(0, Camera->CitizenManager->Religions.Num() - 1);

		Faction.PartyInPower = Camera->CitizenManager->Parties[partyIndex].Party;
		Faction.Religion = Camera->CitizenManager->Religions[religionIndex].Faith;
	}
}

FFactionHappinessStruct& UConquestManager::GetHappinessWithFaction(FFactionStruct Faction, FFactionStruct Target)
{
	FFactionHappinessStruct happiness;
	happiness.Owner = Target.Name;

	int32 index = Faction.Happiness.Find(happiness);

	return Faction.Happiness[index];
}

int32 UConquestManager::GetHappinessValue(FFactionHappinessStruct Happiness)
{
	int32 value = 0;

	for (const TPair<FString, int32>& pair : Happiness.Modifiers)
		value += pair.Value;

	return value;
}

void UConquestManager::SetFactionsHappiness(FFactionStruct& Faction, TArray<FWorldTileStruct*> OccupiedIslands)
{
	FWorldTileStruct* capital = FindCapital(Faction, OccupiedIslands);

	for (FFactionStruct& f : Factions) {
		if (f.Name == Faction.Name)
			continue;

		FFactionHappinessStruct& happiness = GetHappinessWithFaction(Faction, f);
		happiness.ProposalTimer = FMath::Max(happiness.ProposalTimer - 1, 0);

		if (Faction.Religion != f.Religion) {
			int32 value = -18;

			if ((Faction.Religion == "Egg" || Faction.Religion == "Chicken") && (f.Religion == "Egg" || f.Religion == "Chicken"))
				value = -6;

			if (happiness.Contains("Same religion"))
				happiness.RemoveValue("Same religion");

			happiness.SetValue("Different religion", value);
		}
		else {
			if (happiness.Contains("Different religion"))
				happiness.RemoveValue("Different religion");

			happiness.SetValue("Same religion", 18);
		}

		if (Faction.PartyInPower != f.PartyInPower) {
			if (happiness.Contains("Same party"))
				happiness.RemoveValue("Same party");

			happiness.SetValue("Different party", -18);
		}
		else {
			if (happiness.Contains("Different party"))
				happiness.RemoveValue("Different party");

			happiness.SetValue("Same party", 18);
		}

		if (happiness.Contains("Recently enemies"))
			happiness.Decay("Recently enemies");

		if (happiness.Contains("Recently allies"))
			happiness.Decay("Recently allies");

		if (happiness.Contains("Insulted"))
			happiness.Decay("Insulted");

		if (happiness.Contains("Praised"))
			happiness.Decay("Praised");

		if (f.Allies.Contains(Faction.Name)) {
			happiness.SetValue("Allies", 48);
		}
		else if (happiness.Contains("Allies")) {
			happiness.SetValue("Recently allies", -12);
			happiness.RemoveValue("Allies");
		}

		for (FString owner : Faction.Allies) {
			if (happiness.Contains("Recently allied with " + owner))
				happiness.Decay("Recently allied with " + owner);

			if (happiness.Contains("Recently warring with ally: " + owner))
				happiness.Decay("Recently warring with ally: " + owner);

			if (f.Allies.Contains(owner)) {
				happiness.SetValue("Allied with: " + owner, 12);
			}
			else if (happiness.Contains("Allied with " + owner)) {
				happiness.SetValue("Recently allied with " + owner, -6);
				happiness.RemoveValue("Allied with " + owner);
			}

			if (f.AtWar.Contains(owner)) {
				happiness.SetValue("Warring with ally: " + owner, -12);
			}
			else if (happiness.Contains("Warring with ally: " + owner)) {
				happiness.SetValue("Recently wrring with ally: " + owner, -6);
				happiness.RemoveValue("Warring with ally: " + owner);
			}
		}

		if (f.AtWar.Contains(Faction.Name)) {
			happiness.SetValue("Enemies", -48);
		}
		else if (happiness.Contains("Enemies")) {
			happiness.SetValue("Recently enemies", -24);
			happiness.RemoveValue("Enemies");
		}

		for (FString owner : Faction.AtWar) {
			if (happiness.Contains("Recently enemies with " + owner))
				happiness.Decay("Recently enemies with " + owner);

			if (happiness.Contains("Recently allied with enemy: " + owner))
				happiness.Decay("Recently allied with enemy: " + owner);

			if (f.AtWar.Contains(owner)) {
				happiness.SetValue("Enemies with " + owner, 12);
			}
			else if (happiness.Contains("Enemies with " + owner)) {
				happiness.SetValue("Recently enemies with " + owner, 6);
				happiness.RemoveValue("Enemies with " + owner);
			}

			if (f.Allies.Contains(owner)) {
				happiness.SetValue("Allied with enemy: " + owner, -12);
			}
			else if (happiness.Contains("Allied with enemy: " + owner)) {
				happiness.SetValue("Recently allied with enemy: " + owner, 6);
				happiness.RemoveValue("Allied with enemy: " + owner);
			}
		}

		auto bound = FMath::FloorToInt32(FMath::Sqrt((double)WorldSize));
		bool bTooClose = false;

		for (FWorldTileStruct* island : OccupiedIslands) {
			if (island->Owner != f.Name)
				continue;

			int32 distance = FMath::Abs(island->X - capital->X) + FMath::Abs(island->Y - capital->Y);

			if (distance < bound * 0.33f)
				happiness.SetValue(f.Name + "settled on island close to capital", -18);

			bTooClose = true;
		}

		if (!bTooClose && happiness.Contains(f.Name + "settled on island close to capital"))
			happiness.RemoveValue(f.Name + "settled on island close to capital");
	}
}

void UConquestManager::EvaluateDiplomacy(FFactionStruct& Faction)
{
	TArray<FFactionStruct> potentialEnemies;
	TArray<FFactionStruct> potentialAllies;
	
	for (FFactionStruct& f : Factions) {
		if (f.Name == Faction.Name || (f.Name== EmpireName && Portal->HealthComponent->GetHealth() == 0))
			continue;

		FFactionHappinessStruct& happiness = GetHappinessWithFaction(Faction, f);
		FFactionHappinessStruct& fHappiness = GetHappinessWithFaction(f, Faction);

		int32 value = GetHappinessValue(happiness);
		int32 fValue = GetHappinessValue(fHappiness);

		if (value < 12 && Faction.AtWar.IsEmpty() && Faction.Allies.Contains(f.Name)) {
			BreakAlliance(Faction, f);
		}
		else if (Faction.AtWar.Contains(f.Name) && !IsCurrentlyRaiding(Faction, f)) {
			int32 newValue = value;
			int32 newFValue = fValue;

			TTuple<bool, bool> winnability = IsWarWinnable(Faction, f);

			if (winnability.Key)
				newValue -= 24;
			
			if (winnability.Value)
				newFValue -= 24;

			if (newValue + (Faction.WarFatigue / 3) >= 0) {
				if (f.Name == EmpireName) {
					DisplayConquestNotification(Faction.Name + " offers peace", Faction.Name, true);

					happiness.ProposalTimer = 24;
				}
				else if (newFValue + (f.WarFatigue / 3) >= 0)
					Peace(Faction, f);
			}
		}

		if (f.Name == EmpireName)
			fValue = 24;

		if (value < 6) {
			if (Faction.AtWar.IsEmpty() && !Faction.Allies.Contains(f.Name) && Faction.WarFatigue == 0 && IsWarWinnable(Faction, f).Key && !fHappiness.Contains("Recently allies"))
				potentialEnemies.Add(f);
			else if (!fHappiness.Contains("Insulted"))
				Insult(Faction, f);
		}
		else if (value >= 24 && fValue >= 24 && !Faction.Allies.Contains(f.Name) && !Faction.AtWar.Contains(f.Name) && !fHappiness.Contains("Recently allies"))
			potentialAllies.Add(f);
		else if (value > 6 && !fHappiness.Contains("Praised"))
			Praise(Faction, f);

	}

	if (!potentialEnemies.IsEmpty()) {
		int32 index = Camera->Grid->Stream.RandRange(0, potentialEnemies.Num() - 1);
		FFactionStruct f = potentialEnemies[index];

		DeclareWar(Faction, f);

		if (f.Name == EmpireName)
			DisplayConquestNotification(Faction.Name + " has declared war on you", Faction.Name, false);
	}

	if (!potentialAllies.IsEmpty()) {
		int32 index = Camera->Grid->Stream.RandRange(0, potentialAllies.Num() - 1);
		FFactionStruct f = potentialAllies[index];

		if (f.Name == EmpireName) {
			DisplayConquestNotification(Faction.Name + " wants to be your ally", Faction.Name, true);

			FFactionHappinessStruct& happiness = GetHappinessWithFaction(Faction, f);
			happiness.ProposalTimer = 24;
		}
		else
			Ally(Faction, f);
	}
}

TTuple<bool, bool> UConquestManager::IsWarWinnable(FFactionStruct& Faction, FFactionStruct& Target)
{
	int32 factionCitizens = 0;
	int32 targetCitizens = 0;

	for (FWorldTileStruct& tile : World) {
		if (!tile.bIsland || (tile.Owner != Faction.Name && tile.Owner != Target.Name))
			continue;

		TArray<ACitizen*> citizens = GetIslandCitizens(tile);

		if (tile.Owner == Faction.Name)
			factionCitizens += citizens.Num();
		else
			targetCitizens += citizens.Num();
	}

	int32 total = factionCitizens + targetCitizens;

	bool bFactionCanWin = false;
	bool bTargetCanWin = false;

	if (factionCitizens / total > 0.4f)
		bFactionCanWin = true;

	if (targetCitizens / total > 0.4f)
		bTargetCanWin = true;

	return TTuple<bool, bool>(bFactionCanWin, bTargetCanWin);
}

void UConquestManager::Peace(FFactionStruct Faction1, FFactionStruct Faction2)
{
	int32 i1 = Factions.Find(Faction1);
	int32 i2 = Factions.Find(Faction2);

	Factions[i1].AtWar.Remove(Factions[i2].Name);
	Factions[i2].AtWar.Remove(Factions[i1].Name);
}

void UConquestManager::Ally(FFactionStruct Faction1, FFactionStruct Faction2)
{
	int32 i1 = Factions.Find(Faction1);
	int32 i2 = Factions.Find(Faction2);
	
	Factions[i1].Allies.Add(Factions[i2].Name);
	Factions[i2].Allies.Add(Factions[i1].Name);
}

void UConquestManager::BreakAlliance(FFactionStruct Faction1, FFactionStruct Faction2)
{
	int32 i1 = Factions.Find(Faction1);
	int32 i2 = Factions.Find(Faction2);

	Factions[i1].Allies.Remove(Factions[i2].Name);
	Factions[i2].Allies.Remove(Factions[i1].Name);

	FWorldTileStruct factionCapital;
	FWorldTileStruct fCapital;

	TMap<FString, FStationedStruct> stationed;
	stationed.Add(Factions[i1].Name, {});
	stationed.Add(Factions[i2].Name, {});

	for (FWorldTileStruct& tile : World) {
		if (!tile.bIsland || (tile.Owner != Factions[i1].Name && tile.Owner != Factions[i2].Name))
			continue;

		if (tile.bCapital) {
			if (Factions[i1].Name == tile.Owner)
				factionCapital = tile;
			else
				fCapital = tile;
		}
		else {
			for (auto& element : tile.Stationed) {
				if (!stationed.Contains(element.Key))
					continue;

				FStationedStruct* s = stationed.Find(element.Key);
				s->Guards.Append(element.Value.Guards);
			}
		}
	}

	for (auto& element : stationed) {
		FFactionStruct chosenFaction;
		FWorldTileStruct chosenCapital;

		if (element.Key == Factions[i1].Name) {
			chosenFaction = Factions[i1];
			chosenCapital = factionCapital;
		}
		else {
			chosenFaction = Factions[i2];
			chosenCapital = fCapital;
		}

		for (ACitizen* citizen : element.Value.Guards)
			MoveToColony(chosenFaction, chosenCapital, citizen);
	}
}

void UConquestManager::DeclareWar(FFactionStruct Faction1, FFactionStruct Faction2)
{
	int32 i1 = Factions.Find(Faction1);
	int32 i2 = Factions.Find(Faction2);

	Factions[i1].AtWar.Add(Factions[i2].Name);
	Factions[i2].AtWar.Add(Factions[i1].Name);
}

void UConquestManager::Rebel(FFactionStruct& Faction, TArray<FWorldTileStruct*> OccupiedIslands)
{
	FWorldTileStruct* capital = FindCapital(Faction, OccupiedIslands);

	for (FWorldTileStruct* tile : OccupiedIslands) {
		if (tile->bCapital || tile->Owner != Faction.Name)
			continue;

		int32 capitalNum = GetIslandCitizens(*capital).Num();
		int32 tileNum = GetIslandCitizens(*tile).Num();

		double chanceToRebel = tileNum / capitalNum;

		if (chanceToRebel > 0.6f) {
			FString empireNames;
			FFileHelper::LoadFileToString(empireNames, *(FPaths::ProjectDir() + "/Content/Custom/Colony/EmpireNames.txt"));

			TArray<FString> empireParsed;
			empireNames.ParseIntoArray(empireParsed, TEXT(","));

			TArray<FFactionStruct> factionIcons = DefaultOccupierTextureList;

			for (FFactionStruct& faction : Factions) {
				empireParsed.Remove(faction.Name);

				for (int32 i = factionIcons.Num() - 1; i > -1; i--) {
					FFactionStruct icon = factionIcons[i];

					if (faction.Texture == icon.Texture && faction.Colour == icon.Colour)
						factionIcons.RemoveAt(i);
				}
			}

			int32 nameIndex = Camera->Grid->Stream.RandRange(0, empireParsed.Num() - 1);

			FFactionStruct f;
			f.Name = empireParsed[nameIndex];

			if (f.Name == "")
				f.Name = tile->Name + "Empire";

			int32 iconIndex = Camera->Grid->Stream.RandRange(0, factionIcons.Num() - 1);

			f.Texture = factionIcons[iconIndex].Texture;
			f.Colour = factionIcons[iconIndex].Colour;

			FFactionStruct& faction = GetFactionFromOwner(tile->Owner);

			f.AtWar.Add(faction.Name);
			faction.AtWar.Add(f.Name);

			tile->Owner = f.Name;
			Factions.Add(f);
		}
	}
}

void UConquestManager::Insult(FFactionStruct Faction, FFactionStruct Target)
{
	FFactionHappinessStruct& happiness = GetHappinessWithFaction(Target, Faction);

	happiness.SetValue("Insulted", -12);
}

void UConquestManager::Praise(FFactionStruct Faction, FFactionStruct Target)
{
	FFactionHappinessStruct& happiness = GetHappinessWithFaction(Target, Faction);

	happiness.SetValue("Praised", 12);
}

void UConquestManager::Gift(FFactionStruct Faction, TArray<FGiftStruct> Gifts)
{
	for (FGiftStruct gift : Gifts) {
		if (!Camera->ResourceManager->TakeUniversalResource(gift.Resource, gift.Amount, 0))
			continue;

		FFactionStruct& playerFaction = GetFactionFromOwner(EmpireName);

		FFactionHappinessStruct& happiness = GetHappinessWithFaction(Faction, playerFaction);

		int32 value = Camera->ResourceManager->GetMarketValue(gift.Resource) * gift.Amount / 12;

		if (happiness.Contains("Received a gift"))
			happiness.SetValue("Received a gift", happiness.GetValue("Received a gift") + value);
		else
			happiness.SetValue("Received a gift", value);
	}
}

bool UConquestManager::CanBuyIsland(FWorldTileStruct Tile)
{
	if (Tile.Owner == EmpireName || Tile.Owner == "")
		return false;
	
	int32 value = GetIslandWorth(Tile);

	if (value > 1000 || !Tile.RaidParties.IsEmpty())
		return false;

	return true;
}

int32 UConquestManager::GetIslandWorth(FWorldTileStruct Tile)
{
	return Camera->ResourceManager->GetMarketValue(Tile.Resource) * 100 + Tile.Citizens.Num() * 3;
}

void UConquestManager::BuyIsland(FWorldTileStruct Tile)
{
	if (!Camera->ResourceManager->TakeUniversalResource(Camera->ResourceManager->GetResourceFromCategory("Money"), GetIslandWorth(Tile), 0))
		return;

	FWorldTileStruct* island = nullptr;
	int32 index = World.Find(Tile);

	island = &World[index];

	FWorldTileStruct capital;

	FFactionStruct& playerFaction = GetFactionFromOwner(EmpireName);

	for (FWorldTileStruct& tile : World) {
		if (!tile.bIsland || tile.Owner != island->Owner || !tile.bCapital)
			continue;

		capital = tile;

		break;
	}

	for (ACitizen* citizen : island->Citizens)
		MoveToColony(GetFactionFromOwner(island->Owner), capital, citizen);

	for (auto& element : island->Stationed) {
		if (playerFaction.Allies.Contains(element.Key))
			continue;

		for (FWorldTileStruct& tile : World) {
			if (!tile.bIsland || tile.Owner != element.Key || !tile.bCapital)
				continue;

			capital = tile;

			break;
		}

		for (ACitizen* citizen : element.Value.Guards)
			MoveToColony(GetFactionFromOwner(element.Key), capital, citizen);
	}

	island->Owner = playerFaction.Name;
}

//
// Raiding
//
bool UConquestManager::IsCurrentlyRaiding(FFactionStruct Faction1, FFactionStruct Faction2)
{
	for (FWorldTileStruct& tile : World) {
		if (!tile.bIsland || (tile.Owner != Faction1.Name && tile.Owner != Faction2.Name) || tile.RaidParties.IsEmpty())
			continue;

		FRaidStruct raid;

		if (tile.Owner == Faction1.Name)
			raid.Owner = Faction2.Name;
		else
			raid.Owner = Faction1.Name;

		if (tile.RaidParties.Contains(raid))
			return true;
	}

	return false;
}

bool UConquestManager::CanRaidIsland(FFactionStruct Faction, FWorldTileStruct Tile)
{
	if (!Faction.Allies.Contains(Tile.RaidStarterName))
		return false;

	return true;
}

bool UConquestManager::CanStartRaid(FWorldTileStruct* Tile, FFactionStruct* Occupier)
{
	for (FWorldTileStruct& tile : World) {
		if (!tile.bIsland || (tile.Owner != Occupier->Name && !Occupier->Allies.Contains(tile.Owner)))
			continue;

		for (auto& element : tile.Moving) {
			if (element.Value != World.Find(*Tile))
				continue;

			return false;
		}
	}

	return true;
}

void UConquestManager::EvaluateRaid(FWorldTileStruct* Tile)
{
	TArray<ACitizen*> allDefenders;
	allDefenders.Append(Tile->Citizens);

	for (auto& element : Tile->Stationed)
		allDefenders.Append(element.Value.Guards);

	TArray<ACitizen*> allAttackers;
	for (FRaidStruct& party : Tile->RaidParties) {
		TArray<ACitizen*> attackers;
		party.Raiders.GenerateKeyArray(attackers);

		allAttackers.Append(attackers);
	}

	int32 defenders = Camera->Grid->Stream.RandRange(1, FMath::Min(allDefenders.Num() - 1, 20));
	int32 attackers = Camera->Grid->Stream.RandRange(1, FMath::Min(allAttackers.Num() - 1, 20));

	TArray<ACitizen*> defendList;
	TArray<ACitizen*> attackList;
	TArray<ACitizen*> yetToAttackList;

	for (int32 i = 0; i < defenders; i++) {
		int32 index = Camera->Grid->Stream.RandRange(0, allDefenders.Num() - 1);

		defendList.Add(allDefenders[index]);
		yetToAttackList.Add(allDefenders[index]);
	}

	for (int32 i = 0; i < attackers; i++) {
		int32 index = Camera->Grid->Stream.RandRange(0, allAttackers.Num() - 1);

		attackList.Add(allAttackers[index]);
		yetToAttackList.Add(allAttackers[index]);
	}

	float defenderBuff = FMath::Clamp(Tile->HoursColonised / 120.0f, 0.0f, 1.0f) * 0.3f;

	if (Tile->bCapital)
		defenderBuff += 0.2f;

	while (!yetToAttackList.IsEmpty() && !defendList.IsEmpty() && !attackList.IsEmpty()) {
		int32 index = Camera->Grid->Stream.RandRange(0, yetToAttackList.Num() - 1);

		ACitizen* citizen = yetToAttackList[index];
		ACitizen* hitCitizen = nullptr;

		if (defendList.Contains(citizen)) {
			int32 i = Camera->Grid->Stream.RandRange(0, attackList.Num() - 1);

			hitCitizen = attackList[index];
		}
		else {
			int32 i = Camera->Grid->Stream.RandRange(0, defendList.Num() - 1);

			hitCitizen = defendList[index];
		}
		
		hitCitizen->HealthComponent->TakeHealth(citizen->AttackComponent->Damage, citizen);

		yetToAttackList.Remove(citizen);

		if (hitCitizen->HealthComponent->GetHealth() > 0)
			continue;

		if (defendList.Contains(hitCitizen)) {
			defendList.Remove(hitCitizen);

			if (Tile->Citizens.Contains(hitCitizen)) {
				Tile->Citizens.Remove(hitCitizen);
			}
			else {
				for (auto& element : Tile->Stationed) {
					if (!element.Value.Guards.Contains(hitCitizen))
						continue;

					element.Value.Guards.Remove(hitCitizen);

					break;
				}
			}
		}
		else {
			attackList.Remove(hitCitizen);

			for (FRaidStruct& party : Tile->RaidParties) {
				if (!party.Raiders.Contains(hitCitizen))
					continue;

				party.Raiders.Remove(hitCitizen);

				break;
			}
		}

		if (yetToAttackList.Contains(hitCitizen))
			yetToAttackList.Remove(hitCitizen);
	}

	FString logText = "";
	
	if (Tile->Citizens.IsEmpty()) {
		for (FRaidStruct& party : Tile->RaidParties) {
			TArray<ACitizen*> raiders;
			party.Raiders.GenerateKeyArray(raiders);

			if (party.Owner == Tile->RaidStarterName) {
				Tile->Citizens = raiders;
			}
			else {
				FStationedStruct* stationed = Tile->Stationed.Find(party.Owner);
				stationed->Guards.Append(raiders);
			}
		}

		if (Tile->bCapital) {
			FWorldTileStruct* newCapital = nullptr;

			for (FWorldTileStruct& tile : World) {
				if (!tile.bIsland || tile.Owner != Tile->Owner)
					continue;

				if (newCapital == nullptr) {
					newCapital = &tile;

					continue;
				}

				if (newCapital->Citizens.Num() < tile.Citizens.Num())
					newCapital = &tile;
			}

			if (newCapital != nullptr)
				newCapital->bCapital = true;
		}

		Tile->Owner = Tile->RaidStarterName;
		Tile->bCapital = false;

		Camera->UpdateIslandInfoPostRaid(*Tile);

		logText = Tile->Owner + " conquered " + Tile->Name;
	}
	else if (Tile->RaidParties.IsEmpty()) {
		Tile->RaidStarterName = "";

		logText = Tile->Owner + " defended " + Tile->Name;
	}
	
	if (Tile->Citizens.IsEmpty() || Tile->RaidParties.IsEmpty()) {
		Camera->SetIslandBeingRaided(*Tile, false); 
		
		FString type = "Neutral";
		FFactionStruct& playerFaction = GetFactionFromOwner(EmpireName);

		if (Tile->Owner == EmpireName || playerFaction.Allies.Contains(Tile->Owner))
			type = "Good";
		else if (playerFaction.AtWar.Contains(Tile->Owner))
			type = "Bad";

		Camera->NotifyLog(type, logText, Tile->Name);

		Camera->CitizenManager->RemoveTimer("ColonyRaid" + Tile->Name, GetOwner());
	}

	Camera->UpdateRaidHP(*Tile);
}

//
// AI
//
void UConquestManager::EvaluateAI(FFactionStruct& Faction, TArray<FWorldTileStruct*> OccupiedIslands)
{
	TArray<FColoniesStruct> empires;

	for (FWorldTileStruct& tile : World) {
		if (!tile.bIsland)
			continue;

		FColoniesStruct coloniesStruct;

		if (tile.Owner != "")
			coloniesStruct.Faction = &GetFactionFromOwner(tile.Owner);

		int32 index = empires.Find(coloniesStruct);

		if (index == INDEX_NONE) {
			if (tile.bCapital)
				coloniesStruct.Capital = &tile;
			else
				coloniesStruct.Colonies.Add(&tile);

			empires.Add(coloniesStruct);
		}
		else {
			if (tile.bCapital)
				empires[index].Capital = &tile;
			else
				empires[index].Colonies.Add(&tile);
		}
	}

	if (!Faction.AtWar.IsEmpty())
		for (int32 i = empires.Num() - 1; i > -1; i--)
			if (!Faction.AtWar.Contains(empires[i].Faction->Name) && empires[i].Faction->Name != Faction.Name)
				empires.RemoveAt(i);

	FColoniesStruct coloniesStruct;
	coloniesStruct.Faction = &Faction;

	int32 fIndex = empires.Find(coloniesStruct);

	int32 excess = 1000000;

	for (FColoniesStruct e : empires) {
		if (e.Faction->Name == Faction.Name)
			continue;

		int32 fNum = 0;
		int32 eNum = 0;

		for (FWorldTileStruct* tile : empires[fIndex].Colonies)
			fNum += tile->Citizens.Num();

		for (FWorldTileStruct* tile : e.Colonies)
			eNum += tile->Citizens.Num();

		int32 value = fNum - (eNum * 0.8f);

		if (value < excess)
			excess = value;
	}

	FWorldTileStruct* island = nullptr;

	if (Faction.AtWar.IsEmpty()) {
		FColoniesStruct colonyStruct;
		int32 index = empires.Find(colonyStruct);

		for (FWorldTileStruct* colony : empires[index].Colonies) {
			if (island == nullptr) {
				island = colony;

				continue;
			}

			int32 distanceToIsland = FMath::Abs(empires[fIndex].Capital->X - colony->X) + FMath::Abs(empires[fIndex].Capital->Y - colony->Y);
			int32 distanceToColony = FMath::Abs(empires[fIndex].Capital->X - island->X) + FMath::Abs(empires[fIndex].Capital->Y - island->Y);

			if (distanceToIsland > distanceToColony)
				island = colony;
		}
	}
	else {
		for (FColoniesStruct e : empires) {
			for (FWorldTileStruct* colony : e.Colonies) {
				if (island == nullptr) {
					island = colony;

					continue;
				}

				if (island->Citizens.Num() - excess > colony->Citizens.Num() - excess)
					island = colony;
			}
		}
	}

	for (int32 i = 0; i < excess; i++) {
		FWorldTileStruct* tile = empires[fIndex].Capital;

		for (FWorldTileStruct* colony : empires[fIndex].Colonies)
			if ((tile->bCapital && colony->Citizens.Num() / tile->Citizens.Num() > 0.4f) || colony->Citizens.Num() > tile->Citizens.Num())
				tile = colony;

		ACitizen* chosenCitizen = GetChosenCitizen(tile->Citizens);

		MoveToColony(*empires[fIndex].Faction, *island, chosenCitizen);
	}

	for (FWorldTileStruct* colony : empires[fIndex].Colonies) {
		float percentage = colony->Citizens.Num() / empires[fIndex].Capital->Citizens.Num();

		if (percentage > 0.4f) {
			int32 amount = FMath::CeilToInt(colony->Citizens.Num() * percentage);

			for (int32 i = 0; i < amount; i++) {
				ACitizen* chosenCitizen = GetChosenCitizen(colony->Citizens);

				MoveToColony(*empires[fIndex].Faction, *empires[fIndex].Capital, chosenCitizen);
			}
		}
	}
}

ACitizen* UConquestManager::GetChosenCitizen(TArray<ACitizen*> Citizens)
{
	TArray<ACitizen*> males;
	TArray<ACitizen*> females;

	for (ACitizen* citizen : Citizens) {
		if (citizen->BioStruct.Sex == ESex::Male)
			males.Add(citizen);
		else
			females.Add(citizen);
	}

	ACitizen* chosenCitizen = nullptr;

	if (males.Num() > females.Num()) {
		int32 citizenIndex = Camera->Grid->Stream.RandRange(0, males.Num() - 1);
		chosenCitizen = males[citizenIndex];
	}
	else {
		int32 citizenIndex = Camera->Grid->Stream.RandRange(0, females.Num() - 1);
		chosenCitizen = females[citizenIndex];
	}

	return chosenCitizen;
}

//
// UI
//
void UConquestManager::DisplayConquestNotification(FString Message, FString Owner, bool bChoice)
{
	Camera->NotifyConquestEvent(Message, Owner, bChoice);
}

void UConquestManager::SetConquestStatus(bool bEnable)
{
	Portal->SetActorHiddenInGame(!bEnable);
	
	if (bEnable)
		Camera->WorldUIInstance->AddToViewport();
	else
		Camera->WorldUIInstance->RemoveFromParent();
}