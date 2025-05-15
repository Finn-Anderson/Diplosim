#include "Player/Managers/ConquestManager.h"

#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Map/Grid.h"
#include "AI/Citizen.h"
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
		if (!tile.bIsland)
			continue;

		int32 eventChance = FMath::RandRange(0, 100);

		TArray<float> multipliers = { 1.0f, 0.0f, 0.0f };

		if (eventChance == 100)
			multipliers = ProduceEvent();

		if (tile.Occupier.Owner == EmpireName && !tile.bCapital) {
			FIslandResourceStruct islandResource;
			islandResource.ResourceClass = tile.Resource;

			int32 index = ResourceList.Find(islandResource);

			float amount = FMath::RandRange(ResourceList[index].Min, ResourceList[index].Max) * multipliers[0] * tile.Abundance;

			float multiplier = 0.0f;

			for (ACitizen* citizen : tile.Citizens)
				multiplier += citizen->GetProductivity() + (amount * 0.1f);

			Camera->ResourceManager->AddUniversalResource(tile.Resource, amount * (multiplier / tile.Citizens.Num()));

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
		}
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

void UConquestManager::MoveToColony(FWorldTileStruct* Tile, ACitizen* Citizen)
{
	if (!CanTravel(Citizen))
		return;
	
	FWorldTileStruct* tile = GetColonyContainingCitizen(Citizen);
	tile->Moving.Add(TTuple<ACitizen*, FString>(Citizen, Tile->Name));

	if (tile->bCapital && tile->Occupier.Owner == EmpireName) {
		Citizen->AIController->AIMoveTo(Portal);
	}
	else {
		FTimerStruct timer;

		timer.CreateTimer("Transmission", Citizen, FMath::RandRange(10, 40), FTimerDelegate::CreateUObject(this, &UConquestManager::StartTransmissionTimer, Citizen), false);
		Camera->CitizenManager->Timers.Add(timer);
	}
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

	FTimerStruct timer;

	timer.CreateTimer("Travel", Citizen, time, FTimerDelegate::CreateUObject(this, &UConquestManager::AddCitizenToColony, oldTile, targetTile, Citizen), false);
	Camera->CitizenManager->Timers.Add(timer);

	if (oldTile->bCapital && oldTile->Occupier.Owner == EmpireName)
		Citizen->ColonyIslandSetup();
}

void UConquestManager::AddCitizenToColony(FWorldTileStruct* OldTile, FWorldTileStruct* Tile, ACitizen* Citizen)
{
	OldTile->Moving.Remove(Citizen);

	if (Tile->bCapital && Tile->Occupier.Owner == EmpireName) {
		Citizen->MainIslandSetup();

		Citizen->SetActorLocation(Portal->GetActorLocation());
	}

	Tile->Citizens.Add(Citizen);
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

	FWorldTileStruct* nullTile = nullptr;

	return nullTile;
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
	if (Camera->CitizenManager->Injured.Contains(Citizen) || Camera->CitizenManager->Infected.Contains(Citizen) || Citizen->BioStruct.Age < Camera->CitizenManager->GetLawValue(EBillType::WorkAge) || !Citizen->WillWork())
		return false;

	return true;
}

FWorldTileStruct UConquestManager::GetTileInformation(int32 Index)
{
	FWorldTileStruct* tile = &World[Index];

	if (tile->bCapital && tile->Occupier.Owner == EmpireName)
		tile->Citizens = Camera->CitizenManager->Citizens;

	return *tile;
}

TArray<FFactionStruct> UConquestManager::GetFactions()
{
	TArray<FFactionStruct> factions;

	for (FWorldTileStruct& tile : World) {
		if (tile.Occupier.Owner == "" || factions.Contains(tile.Occupier))
			continue;

		factions.Add(tile.Occupier);
	}

	return factions;
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

void UConquestManager::SetTerritoryName(FString OldEmpireName)
{
	FWorldTileStruct* tile = &World[playerCapitalIndex];
	tile->Name = Camera->ColonyName;
	tile->Occupier.Owner = EmpireName;

	Camera->UpdateIconEmpireName(OldEmpireName);
}