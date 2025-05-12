#include "Player/Managers/ConquestManager.h"

#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Map/Grid.h"

UConquestManager::UConquestManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UConquestManager::BeginPlay()
{
	Super::BeginPlay();
	
	Camera = Cast<ACamera>(GetOwner());
}

void UConquestManager::GenerateWorld()
{
	TArray<FWorldTileStruct*> choosableWorldTiles;
	
	for (int32 x = 0; x < 20; x++) {
		auto& row = World.Emplace_GetRef();

		for (int32 y = 0; y < 20; y++) {
			FWorldTileStruct tile;
			tile.X = x;
			tile.Y = y;

			row.Add(tile);
			choosableWorldTiles.Add(&row[y]);
		}
	}

	TArray<FString> factions;
	factions.Add(Camera->ColonyName + " Empire");
	
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
			tile->Males = FMath::RandRange(25, 50);
			tile->Females = FMath::RandRange(25, 50);

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
	}

	while (!choosableWorldTiles.IsEmpty()) {
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
	}
}

void UConquestManager::GiveResource()
{
	for (TArray<FWorldTileStruct>& row : World) {
		for (FWorldTileStruct& tile : row) {
			if (!tile.bIsland)
				continue;

			int32 eventChance = FMath::RandRange(0, 100);

			TArray<int32> multipliers = { 1, 1 };

			if (eventChance == 100)
				multipliers = ProduceEvent();

			if (tile.Owner == Camera->ColonyName && !tile.bCapital) {
				FIslandResourceStruct islandResource;
				islandResource.ResourceClass = tile.Resource;

				int32 index = ResourceList.Find(islandResource);

				Camera->ResourceManager->AddUniversalResource(tile.Resource, FMath::RandRange(ResourceList[index].Min, ResourceList[index].Max) * multipliers[0]);
			}

			int32 num = tile.Males;

			if (num > tile.Females)
				num = tile.Females;

			num *= FMath::CeilToInt32(FMath::FRandRange(0.0f, (num * 0.1f)) * multipliers[1]);

			int32 maleRatio = FMath::FRandRange(0.0f, 1.0f);

			int32 males = num * maleRatio;

			tile.Males = males;
			tile.Females = num - males;
		}
	}
}

TArray<int32> UConquestManager::ProduceEvent()
{
	int32 index = FMath::RandRange(0, EventList.Num() - 1);

	return { EventList[index].ResourceMultiplier, EventList[index].CitizenMultiplier };
}