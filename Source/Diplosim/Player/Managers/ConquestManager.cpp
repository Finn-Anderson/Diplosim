#include "Player/Managers/ConquestManager.h"

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
	
	FString emprieNames;
	FFileHelper::LoadFileToString(emprieNames, *(FPaths::ProjectDir() + "/Content/Custom/Colony/EmpireNames.txt"));

	TArray<FString> emprieParsed;
	emprieNames.ParseIntoArray(emprieParsed, TEXT(","));

	FString colonyNames;
	FFileHelper::LoadFileToString(colonyNames, *(FPaths::ProjectDir() + "/Content/Custom/Colony/ColonyNames.txt"));

	TArray<FString> colonyParsed;
	colonyNames.ParseIntoArray(colonyParsed, TEXT(","));

	for (int32 i = 0; i < EnemiesNum; i++) {
		int32 index = FMath::RandRange(0, emprieParsed.Num() - 1);
		factions.Add(emprieParsed[index]);

		if (emprieParsed[index] != "")
			emprieParsed.RemoveAt(index);
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
		tile->Occupier.Owner = name;

		int32 tIndex = Camera->Grid->Stream.RandRange(0, factionIcons.Num() - 1);

		tile->Occupier.Texture = factionIcons[tIndex].Texture;
		tile->Occupier.Colour = factionIcons[tIndex].Colour;

		factionIcons.RemoveAt(tIndex);

		int32 rIndex = Camera->Grid->Stream.RandRange(0, ResourceList.Num() - 1);
		tile->Resource = ResourceList[rIndex].ResourceClass;

		tile->Abundance = Camera->Grid->Stream.RandRange(1, 5);

		if (name != EmpireName) {
			int32 cIndex = FMath::RandRange(0, colonyParsed.Num() - 1);
			tile->Name = colonyParsed[cIndex];

			if (name == "")
				tile->Occupier.Owner = tile->Name + "Empire";

			colonyParsed.RemoveAt(cIndex);
		}
		else {
			tile->Name = Camera->ColonyName;

			playerCapitalIndex = index;
		}

		choosableWorldTiles.RemoveAt(index);
		islandsNum--;
	}

	while (islandsNum > 0) {
		int32 index = Camera->Grid->Stream.RandRange(0, choosableWorldTiles.Num() - 1);
		FWorldTileStruct* tile = choosableWorldTiles[index];

		tile->bIsland = true;

		if (!colonyParsed.IsEmpty()) {
			int32 cIndex = FMath::RandRange(0, colonyParsed.Num() - 1);
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
	for (FWorldTileStruct& tile : World) {
		if (tile.Occupier.Owner == "" || tile.Occupier.Owner == EmpireName)
			continue;

		for (int32 i = 0; i < FMath::RandRange(25, 50); i++)
			SpawnCitizenAtColony(&tile);
	}
}

void UConquestManager::GiveResource()
{
	for (FWorldTileStruct& tile : World) {
		if (!tile.bIsland || tile.Occupier.Owner == "")
			continue;

		int32 eventChance = FMath::RandRange(0, 100);

		TArray<float> multipliers = { 1.0f, 0.0f, 0.0f };

		if (eventChance == 100)
			multipliers = ProduceEvent();

		int32 value = 0;
		bool bNegative = false;

		if (multipliers[1] != 0.0f) {
			value = FMath::FRandRange(0.0f, FMath::Abs(multipliers[1])) * tile.Citizens.Num();

			if (multipliers[1] < 0.0f)
				bNegative = true;
		}
		else if (multipliers[2] != 0.0f) {
			value = FMath::RandRange(0, FMath::Abs((int32)multipliers[2]));

			if (multipliers[2] < 0.0f)
				bNegative = true;
		}

		if (value > 0)
			ModifyCitizensEvent(&tile, value, bNegative);

		tile.HoursColonised++;

		if (tile.Occupier.Owner != EmpireName || tile.bCapital)
			continue;

		FIslandResourceStruct islandResource;
		islandResource.ResourceClass = tile.Resource;

		int32 index = ResourceList.Find(islandResource);

		float amount = FMath::RandRange(ResourceList[index].Min, ResourceList[index].Max) * multipliers[0] * tile.Abundance;

		float multiplier = 0.0f;

		for (ACitizen* citizen : tile.Citizens)
			multiplier += citizen->GetProductivity() + (amount * 0.1f);

		Camera->ResourceManager->AddUniversalResource(tile.Resource, amount * (multiplier / tile.Citizens.Num()));
	}
	for (FFactionStruct* faction : GetFactions()) {
		if (faction->AtWar.IsEmpty() && faction->WarFatigue > 0)
			faction->WarFatigue--;
		else if (!faction->AtWar.IsEmpty())
			faction->WarFatigue++;

		if (faction->Owner == EmpireName)
			continue;

		SetFactionsHappiness(faction);

		EvaluateDiplomacy(faction);
	}
}

void UConquestManager::SpawnCitizenAtColony(FWorldTileStruct* Tile)
{
	FActorSpawnParameters params;
	params.bNoFail = true;

	ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(CitizenClass, FVector(0.0f, 0.0f, -10000.0f), FRotator::ZeroRotator, params);

	citizen->SetSex(Tile->Citizens);

	for (int32 j = 0; j < 2; j++)
		citizen->GivePersonalityTrait();

	citizen->BioStruct.Age = 17;
	citizen->Birthday();

	citizen->HealthComponent->AddHealth(100);

	citizen->ApplyResearch();

	citizen->SetActorHiddenInGame(true);

	Tile->Citizens.Add(citizen);
}

void UConquestManager::MoveToColony(FFactionStruct Faction, FWorldTileStruct* Tile, ACitizen* Citizen)
{
	if (!CanTravel(Citizen))
		return;

	if (Tile->Occupier.Owner == "")
		Tile->Occupier = Faction;
	else if (Tile->Occupier != Faction && Tile->RaidStarter == nullptr)
		Tile->RaidStarter = &Faction;
	
	FWorldTileStruct* tile = GetColonyContainingCitizen(Citizen);
	tile->Moving.Add(Citizen, Tile->Name);

	if (tile->bCapital && tile->Occupier.Owner == EmpireName)
		Citizen->AIController->AIMoveTo(Portal);
	else
		Camera->CitizenManager->CreateTimer("Transmission", Citizen, FMath::RandRange(10, 40), FTimerDelegate::CreateUObject(this, &UConquestManager::StartTransmissionTimer, Citizen), false);
}

void UConquestManager::StartTransmissionTimer(ACitizen* Citizen)
{
	FWorldTileStruct* oldTile = GetColonyContainingCitizen(Citizen);

	if (!CanTravel(Citizen)) {
		oldTile->Moving.Remove(Citizen);

		if (oldTile->bCapital && oldTile->Occupier.Owner == EmpireName)
			Citizen->AIController->DefaultAction();

		return;
	}

	FString islandName = *oldTile->Moving.Find(Citizen);

	FWorldTileStruct* targetTile = nullptr;

	for (FWorldTileStruct& tile : World) {
		if (tile.Name != islandName)
			continue;

		targetTile = &tile;

		break;
	}

	oldTile->Citizens.Remove(Citizen);

	int32 x = FMath::Abs(oldTile->X - targetTile->X);
	int32 y = FMath::Abs(oldTile->Y - targetTile->Y);

	int32 time = (x + y) * 10;

	Camera->CitizenManager->CreateTimer("Travel", Citizen, time, FTimerDelegate::CreateUObject(this, &UConquestManager::AddCitizenToColony, oldTile, targetTile, Citizen), false);

	if (oldTile->bCapital && oldTile->Occupier.Owner == EmpireName)
		Citizen->ColonyIslandSetup();
}

void UConquestManager::AddCitizenToColony(FWorldTileStruct* OldTile, FWorldTileStruct* Tile, ACitizen* Citizen)
{
	OldTile->Moving.Remove(Citizen);

	if (Tile->Occupier != OldTile->Occupier) {
		int32 index = World.Find(*OldTile);

		Tile->Raiding.Add(Citizen, index);

		if (CanStartRaid(Tile, OldTile->Occupier))
			Camera->CitizenManager->CreateTimer("ColonyRaid" + Tile->Name, GetOwner(), 1.0f, FTimerDelegate::CreateUObject(this, &UConquestManager::EvaluateRaid, Tile), true);
	}
	else if (Tile->bCapital && Tile->Occupier.Owner == EmpireName) {
		Citizen->MainIslandSetup();

		Citizen->SetActorLocation(Portal->GetActorLocation());
	}
	else {
		Tile->Citizens.Add(Citizen);
	}

	Camera->CitizenManager->CreateTimer("RecentlyMoved", Citizen, 300, FTimerDelegate::CreateUObject(this, &UConquestManager::RemoveFromRecentlyMoved, Citizen), false);
}

TArray<float> UConquestManager::ProduceEvent()
{
	int32 index = FMath::RandRange(0, EventList.Num() - 1);

	return { EventList[index].ResourceMultiplier, EventList[index].CitizenMultiplier, EventList[index].Citizens };
}

FWorldTileStruct* UConquestManager::GetColonyContainingCitizen(ACitizen* Citizen)
{
	for (FWorldTileStruct& tile : World) {
		if (!tile.bIsland || !tile.Citizens.Contains(Citizen))
			continue;

		return &tile;
	}

	return nullptr;
}

void UConquestManager::ModifyCitizensEvent(FWorldTileStruct* Tile, int32 Amount, bool bNegative)
{
	for (int32 i = 0; i < Amount; i++) {
		if (bNegative) {
			int32 index = FMath::RandRange(0, Tile->Citizens.Num() - 1);

			Tile->Citizens[index]->HealthComponent->TakeHealth(1000, Tile->Citizens[index]);
		}
		else {
			SpawnCitizenAtColony(Tile);
		}
	}
}

bool UConquestManager::CanTravel(class ACitizen* Citizen)
{
	if (RecentlyMoved.Contains(Citizen) || Camera->CitizenManager->Injured.Contains(Citizen) || Camera->CitizenManager->Infected.Contains(Citizen) || Citizen->BioStruct.Age < Camera->CitizenManager->GetLawValue(EBillType::WorkAge) || !Citizen->WillWork())
		return false;

	return true;
}

FWorldTileStruct UConquestManager::GetTileInformation(int32 Index)
{
	FWorldTileStruct tile = World[Index];

	if (tile.bCapital && tile.Occupier.Owner == EmpireName)
		tile.Citizens = Camera->CitizenManager->Citizens;

	return tile;
}

TArray<FFactionStruct*> UConquestManager::GetFactions()
{
	TArray<FFactionStruct*> factions;

	for (FWorldTileStruct& tile : World) {
		if (tile.Occupier.Owner == "" || factions.Contains(tile.Occupier))
			continue;

		factions.Add(&tile.Occupier);
	}

	return factions;
}

FFactionStruct* UConquestManager::GetFactionFromOwner(FString Owner)
{
	for (FFactionStruct* faction : GetFactions()) {
		if (faction->Owner != Owner)
			continue;

		return faction;
	}

	return nullptr;
}

void UConquestManager::SetFactionTexture(FFactionStruct Faction, UTexture2D* Texture, FLinearColor Colour)
{
	for (FWorldTileStruct& tile : World) {
		if (tile.Occupier.Owner != Faction.Owner)
			continue;

		tile.Occupier.Texture = Texture;
		tile.Occupier.Colour = Colour;
	}

	Camera->UpdateFactionImage();
}

void UConquestManager::SetColonyName(FWorldTileStruct* Tile, FString NewColonyName)
{
	Tile->Name = NewColonyName;
}

void UConquestManager::SetTerritoryName(FString OldEmpireName)
{
	FWorldTileStruct* tile = &World[playerCapitalIndex];
	tile->Name = Camera->ColonyName;
	tile->Occupier.Owner = EmpireName;

	Camera->UpdateIconEmpireName(OldEmpireName);
}

void UConquestManager::RemoveFromRecentlyMoved(class ACitizen* Citizen)
{
	RecentlyMoved.Remove(Citizen);
}

TArray<ACitizen*> UConquestManager::GetIslandCitizens(FWorldTileStruct* Tile)
{
	if (Tile->bCapital && Tile->Occupier.Owner == EmpireName)
		return Camera->CitizenManager->Citizens;
	else
		return Tile->Citizens;
}

//
// Diplomacy
//
FFactionHappinessStruct* UConquestManager::GetHappinessWithFaction(FFactionStruct* Faction, FFactionStruct* Target)
{
	FFactionHappinessStruct happiness;
	happiness.Owner = Target->Owner;

	int32 index = Faction->Happiness.Find(happiness);

	return &Faction->Happiness[index];
}

int32 UConquestManager::GetHappinessValue(FFactionHappinessStruct* Happiness)
{
	int32 value = 0;

	for (const TPair<FString, int32>& pair : Happiness->Modifiers)
		value += pair.Value;

	return value;
}

void UConquestManager::SetFactionsHappiness(FFactionStruct* Faction)
{
	for (FFactionStruct* f : GetFactions()) {
		if (f->Owner == Faction->Owner)
			continue;

		FFactionHappinessStruct* happiness = GetHappinessWithFaction(Faction, f);

		if (happiness->Contains("Recently enemies"))
			happiness->Decay("Recently enemies");

		if (happiness->Contains("Recently allies"))
			happiness->Decay("Recently allies");

		if (happiness->Contains("Insulted"))
			happiness->Decay("Insulted");

		if (happiness->Contains("Praised"))
			happiness->Decay("Praised");

		if (f->Allies.Contains(Faction->Owner)) {
			happiness->SetValue("Allies", 48);
		}
		else if (happiness->Contains("Allies")) {
			happiness->SetValue("Recently allies", -12);
			happiness->RemoveValue("Allies");
		}

		for (FString owner : Faction->Allies) {
			if (happiness->Contains("Recently allied with " + owner))
				happiness->Decay("Recently allied with " + owner);

			if (happiness->Contains("Recently warring with ally: " + owner))
				happiness->Decay("Recently warring with ally: " + owner);

			if (f->Allies.Contains(owner)) {
				happiness->SetValue("Allied with: " + owner, 12);
			}
			else if (happiness->Contains("Allied with " + owner)) {
				happiness->SetValue("Recently allied with " + owner, -6);
				happiness->RemoveValue("Allied with " + owner);
			}

			if (f->AtWar.Contains(owner)) {
				happiness->SetValue("Warring with ally: " + owner, -12);
			}
			else if (happiness->Contains("Warring with ally: " + owner)) {
				happiness->SetValue("Recently wrring with ally: " + owner, -6);
				happiness->RemoveValue("Warring with ally: " + owner);
			}
		}

		if (f->AtWar.Contains(Faction->Owner)) {
			happiness->SetValue("Enemies", -48);
		}
		else if (happiness->Contains("Enemies")) {
			happiness->SetValue("Recently enemies", -24);
			happiness->RemoveValue("Enemies");
		}

		for (FString owner : Faction->AtWar) {
			if (happiness->Contains("Recently enemies with " + owner))
				happiness->Decay("Recently enemies with " + owner);

			if (happiness->Contains("Recently allied with enemy: " + owner))
				happiness->Decay("Recently allied with enemy: " + owner);

			if (f->AtWar.Contains(owner)) {
				happiness->SetValue("Enemies with " + owner, 12);
			}
			else if (happiness->Contains("Enemies with " + owner)) {
				happiness->SetValue("Recently enemies with " + owner, 6);
				happiness->RemoveValue("Enemies with " + owner);
			}

			if (f->Allies.Contains(owner)) {
				happiness->SetValue("Allied with enemy: " + owner, -12);
			}
			else if (happiness->Contains("Allied with enemy: " + owner)) {
				happiness->SetValue("Recently allied with enemy: " + owner, 6);
				happiness->RemoveValue("Allied with enemy: " + owner);
			}
		}
	}
}

void UConquestManager::EvaluateDiplomacy(FFactionStruct* Faction)
{
	TArray<FFactionStruct*> potentialEnemies;
	TArray<FFactionStruct*> potentialAllies;
	
	for (FFactionStruct* f : GetFactions()) {
		if (f->Owner == Faction->Owner)
			continue;

		FFactionHappinessStruct* happiness = GetHappinessWithFaction(Faction, f);
		FFactionHappinessStruct* fHappiness = GetHappinessWithFaction(f, Faction);

		int32 value = GetHappinessValue(happiness);
		int32 fValue = GetHappinessValue(fHappiness);

		if (value < 25 && Faction->AtWar.IsEmpty() && Faction->Allies.Contains(f->Owner)) {
			Faction->Allies.Remove(f->Owner);
			f->Allies.Remove(Faction->Owner);
		}
		else if (Faction->AtWar.Contains(f)) {
			int32 newValue = value;
			int32 newFValue = fValue;

			TTuple<bool, bool> winnability = IsWarWinnable(Faction, f);

			if (winnability.Key)
				newValue -= 25;
			
			if (winnability.Value)
				newFValue -= 25;

			if (newValue + (Faction->WarFatigue / 3) >= 0) {
				if (f->Owner == EmpireName) {
					// UI proposal that links to Peace()
				}
				else if (newFValue + (f->WarFatigue / 3) >= 0)
					Peace(Faction, f);
			}
		}

		if (f->Owner == EmpireName)
			fValue = 25;

		if (value <= -25 && Faction->AtWar.IsEmpty() && Faction->WarFatigue == 0 && IsWarWinnable(Faction, f).Key)
			potentialEnemies.Add(f);
		else if (value >= 25 && fValue >= 25 && !Faction->Allies.Contains(f->Owner))
			potentialAllies.Add(f);
	}

	if (!potentialEnemies.IsEmpty()) {
		int32 index = FMath::RandRange(0, potentialEnemies.Num() - 1);
		FFactionStruct* f = potentialEnemies[index];

		Faction->AtWar.Add(f->Owner);
		f->AtWar.Add(Faction->Owner);
	}

	if (!potentialAllies.IsEmpty()) {
		int32 index = FMath::RandRange(0, potentialAllies.Num() - 1);
		FFactionStruct* f = potentialAllies[index];

		if (f->Owner == EmpireName) {
			// UI proposal that links to Ally()
		}
		else
			Ally(Faction, f);
	}
}

TTuple<bool, bool> UConquestManager::IsWarWinnable(FFactionStruct* Faction, FFactionStruct* Target)
{
	int32 factionCitizens = 0;
	int32 targetCitizens = 0;

	for (FWorldTileStruct& tile : World) {
		if (!tile.bIsland || (tile.Occupier != *Faction && tile.Occupier != *Target))
			continue;

		TArray<ACitizen*> citizens = GetIslandCitizens(&tile);

		if (tile.Occupier == *Faction)
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

void UConquestManager::Peace(FFactionStruct* Faction1, FFactionStruct* Faction2)
{
	Faction1->AtWar.Remove(Faction2->Owner);
	Faction2->AtWar.Remove(Faction1->Owner);
}

void UConquestManager::Ally(FFactionStruct* Faction1, FFactionStruct* Faction2)
{
	Faction1->Allies.Add(Faction2->Owner);
	Faction2->Allies.Add(Faction1->Owner);
}

//
// Raiding
//
bool UConquestManager::CanStartRaid(FWorldTileStruct* Tile, FFactionStruct Occupier)
{
	for (FWorldTileStruct& tile : World) {
		if (!tile.bIsland || (tile.Occupier != Occupier && !Occupier.Allies.Contains(tile.Occupier.Owner)))
			continue;

		for (auto& element : tile.Moving) {
			if (element.Value != Tile)
				continue;

			return false;
		}
	}

	return true;
}

void UConquestManager::EvaluateRaid(FWorldTileStruct* Tile)
{
	int32 defenders = FMath::RandRange(1, FMath::Min(Tile->Citizens.Num() - 1, 20));
	int32 attackers = FMath::RandRange(1, FMath::Min(Tile->Raiding.Num() - 1, 20));

	TArray<ACitizen*> defendList;
	TArray<ACitizen*> attackList;
	TArray<ACitizen*> yetToAttackList;

	for (int32 i = 0; i < defenders; i++) {
		int32 index = FMath::RandRange(0, Tile->Citizens.Num() - 1);

		defendList.Add(Tile->Citizens[index]);
		yetToAttackList.Add(Tile->Citizens[index]);
	}

	for (int32 i = 0; i < defenders; i++) {
		TArray<ACitizen*> raiders;
		Tile->Raiding.GenerateKeyArray(raiders);

		int32 index = FMath::RandRange(0, raiders.Num() - 1);

		attackList.Add(raiders[index]);
		yetToAttackList.Add(raiders[index]);
	}

	float defenderBuff = FMath::Clamp(Tile->HoursColonised / 120.0f, 0.0f, 1.0f) * 0.3f;

	while (!yetToAttackList.IsEmpty() && !defendList.IsEmpty() && !attackList.IsEmpty()) {
		int32 index = FMath::RandRange(0, yetToAttackList.Num() - 1);

		ACitizen* citizen = yetToAttackList[index];
		ACitizen* hitCitizen = nullptr;

		if (defendList.Contains(citizen)) {
			int32 i = FMath::RandRange(0, attackList.Num() - 1);

			hitCitizen = attackList[index];
		}
		else {
			int32 i = FMath::RandRange(0, defendList.Num() - 1);

			hitCitizen = defendList[index];
		}
		
		hitCitizen->HealthComponent->TakeHealth(citizen->AttackComponent->Damage, citizen);

		yetToAttackList.Remove(citizen);

		if (hitCitizen->HealthComponent->GetHealth() > 0)
			continue;

		if (defendList.Contains(hitCitizen)) {
			defendList.Remove(hitCitizen);
			Tile->Citizens.Remove(hitCitizen);
		}
		else {
			attackList.Remove(hitCitizen);
			Tile->Raiding.Remove(hitCitizen);
		}

		if (yetToAttackList.Contains(hitCitizen))
			yetToAttackList.Remove(hitCitizen);
	}
	
	if (Tile->Citizens.IsEmpty()) {
		for (auto& element : Tile->Raiding) {
			FWorldTileStruct* oldTile = &World[element.Value];

			if (oldTile->Occupier == *Tile->RaidStarter)
				Tile->Citizens.Add(element.Key);
			else
				AddCitizenToColony(Tile, oldTile, element.Key);
		}

		Tile->Raiding.Empty();
	}
	else if (Tile->Raiding.IsEmpty()) {
		Tile->RaidStarter = nullptr;
	}
	
	if (Tile->Citizens.IsEmpty() || Tile->Raiding.IsEmpty())
		Camera->CitizenManager->RemoveTimer("ColonyRaid" + Tile->Name, GetOwner());
}