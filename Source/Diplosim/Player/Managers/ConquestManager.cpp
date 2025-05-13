#include "Player/Managers/ConquestManager.h"

#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Map/Grid.h"
#include "AI/Citizen.h"
#include "Universal/HealthComponent.h"

UConquestManager::UConquestManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	EmpireName = "";
	WorldSize = 400;
	PercentageIsland = 10;
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
		auto& row = World.Emplace_GetRef();

		for (int32 y = 0; y < bound; y++) {
			FWorldTileStruct tile;
			tile.X = x;
			tile.Y = y;

			row.Add(tile);
			choosableWorldTiles.Add(&row[y]);
		}
	}

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

	for (int32 i = 0; i < 2; i++) {
		int32 index = FMath::RandRange(0, emprieParsed.Num() - 1);
		factions.Add(emprieParsed[index]);

		if (emprieParsed[index] != "")
			emprieParsed.RemoveAt(index);
	}

	int32 islandsNum = choosableWorldTiles.Num() * (PercentageIsland / 100.0f);

	for (FString name : factions) {
		int32 index = Camera->Grid->Stream.RandRange(0, choosableWorldTiles.Num() - 1);
		FWorldTileStruct* tile = choosableWorldTiles[index];

		tile->bIsland = true;
		tile->bCapital = true;
		tile->Owner = name;

		int32 rIndex = Camera->Grid->Stream.RandRange(0, ResourceList.Num() - 1);
		tile->Resource = ResourceList[rIndex].ResourceClass;

		tile->Abundance = Camera->Grid->Stream.RandRange(0, 3);

		if (name != Camera->ColonyName) {
			int32 cIndex = FMath::RandRange(0, colonyParsed.Num() - 1);
			tile->Name = colonyParsed[cIndex];

			if (name == "")
				tile->Owner = tile->Name + "Empire";

			colonyParsed.RemoveAt(cIndex);
		}
		else {
			tile->Name = Camera->ColonyName;
		}

		choosableWorldTiles.RemoveAt(index);
		islandsNum--;
	}

	while (!choosableWorldTiles.IsEmpty() || islandsNum > 0) {
		int32 index = Camera->Grid->Stream.RandRange(0, choosableWorldTiles.Num() - 1);
		FWorldTileStruct* tile = choosableWorldTiles[index];

		int32 chance = Camera->Grid->Stream.RandRange(0, 10);

		if (chance > 9 && !colonyParsed.IsEmpty()) {
			tile->bIsland = true;

			int32 cIndex = FMath::RandRange(0, colonyParsed.Num() - 1);
			tile->Name = colonyParsed[cIndex];

			int32 rIndex = Camera->Grid->Stream.RandRange(0, ResourceList.Num() - 1);
			tile->Resource = ResourceList[rIndex].ResourceClass;

			tile->Abundance = Camera->Grid->Stream.RandRange(0, 3);
		}

		choosableWorldTiles.RemoveAt(index);
		islandsNum--;
	}
}

void UConquestManager::StartConquest()
{
	for (TArray<FWorldTileStruct>& row : World) {
		for (FWorldTileStruct& tile : row) {
			if (tile.Owner == "" || tile.Owner == EmpireName)
				continue;

			for (int32 i = 0; i < FMath::RandRange(25, 50); i++)
				SpawnCitizenAtColony(&tile);
		}
	}
}

void UConquestManager::GiveResource()
{
	for (TArray<FWorldTileStruct>& row : World) {
		for (FWorldTileStruct& tile : row) {
			if (!tile.bIsland)
				continue;

			int32 eventChance = FMath::RandRange(0, 100);

			TArray<float> multipliers = { 1.0f, 0.0f, 0.0f };

			if (eventChance == 100)
				multipliers = ProduceEvent();

			if (tile.Owner == Camera->ColonyName && !tile.bCapital) {
				FIslandResourceStruct islandResource;
				islandResource.ResourceClass = tile.Resource;

				int32 index = ResourceList.Find(islandResource);

				float amount = FMath::RandRange(ResourceList[index].Min, ResourceList[index].Max) * multipliers[0];

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
	FWorldTileStruct* tile = GetColonyContainingCitizen(Citizen);
	tile->Moving.Add(TTuple<ACitizen*, FString>(Citizen, Tile->Name));

	if (tile->bCapital && tile->Owner == EmpireName) {
		// Move to portal
	}
	else {
		StartTransmissionTimer(Citizen, FMath::RandRange(10, 40));
	}
}

void UConquestManager::StartTransmissionTimer(ACitizen* Citizen, int32 Time)
{
	FWorldTileStruct* oldTile = GetColonyContainingCitizen(Citizen);
	FString islandName = *oldTile->Moving.Find(Citizen);

	FWorldTileStruct* targetTile = nullptr;

	for (TArray<FWorldTileStruct>& row : World) {
		for (FWorldTileStruct& tile : row) {
			if (tile.Name != islandName)
				continue;

			targetTile = &tile;

			break;
		}
	}

	oldTile->Citizens.Remove(Citizen);

	int32 x = FMath::Abs(oldTile->X - targetTile->X);
	int32 y = FMath::Abs(oldTile->Y - targetTile->Y);

	int32 time = Time + (x + y) * 10;

	FTimerStruct timer;

	timer.CreateTimer("Travel", Citizen, time, FTimerDelegate::CreateUObject(this, &UConquestManager::AddCitizenToColony, oldTile, targetTile, Citizen), false);
	Camera->CitizenManager->Timers.Add(timer);
}

void UConquestManager::AddCitizenToColony(FWorldTileStruct* OldTile, FWorldTileStruct* Tile, ACitizen* Citizen)
{
	OldTile->Moving.Remove(Citizen);

	if (Tile->bCapital && Tile->Owner == EmpireName) {
		Citizen->MainIslandSetup();

		// Set actor location at portal
	}
	else {
		Tile->Citizens.Add(Citizen);

		Citizen->ColonyIslandSetup();
	}
}

TArray<float> UConquestManager::ProduceEvent()
{
	int32 index = FMath::RandRange(0, EventList.Num() - 1);

	return { EventList[index].ResourceMultiplier, EventList[index].CitizenMultiplier, EventList[index].Citizens };
}

FWorldTileStruct* UConquestManager::GetColonyContainingCitizen(ACitizen* Citizen)
{
	for (TArray<FWorldTileStruct>& row : World) {
		for (FWorldTileStruct& tile : row) {
			if (!tile.bIsland || !tile.Citizens.Contains(Citizen))
				continue;

			return &tile;
		}
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