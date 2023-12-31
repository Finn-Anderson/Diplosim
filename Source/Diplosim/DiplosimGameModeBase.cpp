#include "DiplosimGameModeBase.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NavigationSystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "Map/Grid.h"
#include "Buildings/Broch.h"
#include "HealthComponent.h"

ADiplosimGameModeBase::ADiplosimGameModeBase()
{
	earliestSpawnTime = 900;
	latestSpawnTime = 1800;
}

bool ADiplosimGameModeBase::PathToBroch(class AGrid* Grid, struct FTileStruct tile, bool bCheckLength)
{
	if (tile.Level < 0)
		return false;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

	FTransform transform;
	Grid->HISMGround->GetInstanceTransform(tile.Instance, transform);

	float z = Grid->HISMGround->GetStaticMesh()->GetBoundingBox().GetSize().Z;

	FVector location = transform.GetLocation() + FVector(0.0f, 0.0f, z);

	FPathFindingQuery query(this, *NavData, location, Broch->BuildingMesh->GetSocketLocation("Entrance"));

	bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

	if (!path)
		return false;

	if (bCheckLength) {
		FVector::FReal outLength;
		NavData->CalcPathLength(location, Broch->BuildingMesh->GetSocketLocation("Entrance"), outLength);

		if (outLength < 3000.0f)
			return false;
	}

	return true;
}

TArray<FVector> ADiplosimGameModeBase::GetSpawnPoints(class AGrid* Grid, bool bCheckLength)
{
	TArray<FVector> validTiles;

	for (FTileStruct tile : Grid->Storage) {
		if (tile.Level < 0)
			continue;

		if (!PathToBroch(Grid, tile, bCheckLength))
			continue;

		FTransform transform;
		Grid->HISMGround->GetInstanceTransform(tile.Instance, transform);

		float z = Grid->HISMGround->GetStaticMesh()->GetBoundingBox().GetSize().Z;

		validTiles.Add(transform.GetLocation() + FVector(0.0f, 0.0f, z));
	}

	if (validTiles.IsEmpty()) {
		validTiles = GetSpawnPoints(Grid, !bCheckLength);
	}

	return validTiles;
}

TArray<FVector> ADiplosimGameModeBase::PickSpawnPoints()
{
	TArray<AActor*> grids;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrid::StaticClass(), grids);

	AGrid* grid = Cast<AGrid>(grids[0]);

	TArray<FVector> validTiles;

	for (FWaveStruct wave : WavesData) {
		if (wave == WavesData.Last())
			continue;

		if (wave.DiedTo.Num() * 0.66f > wave.NumKilled) {
			for (FVector location : validTiles) {
				if (location != wave.SpawnLocation)
					continue;

				validTiles.RemoveSingle(location);

				break;
			}

			continue;
		}

		validTiles.Add(wave.SpawnLocation);
	}

	if (validTiles.IsEmpty()) {
		validTiles = GetSpawnPoints(grid, true);
	}

	int32 index = FMath::RandRange(0, validTiles.Num() - 1);

	TArray<FVector> chosenLocations;
	chosenLocations.Add(validTiles[index]);

	WavesData.Last().SpawnLocation = validTiles[index];

	FTileStruct chosenTile = grid->Storage[validTiles[index].X / 100 + 100 + (validTiles[index].Y / 100 + 100) * grid->Size];

	int32 z = grid->HISMGround->GetStaticMesh()->GetBoundingBox().GetSize().Z;

	for (int32 y = -4; y < 5; y++)
	{
		for (int32 x = -4; x < 5; x++)
		{
			if (chosenTile.X + x < 0 || chosenTile.Y + y < 0 || chosenTile.X + x >= grid->Size || chosenTile.Y + y >= grid->Size)
				continue;

			FTileStruct tile = grid->Storage[(chosenTile.X - x) + ((chosenTile.Y - y) * grid->Size)];

			if (PathToBroch(grid, tile, false)) {
				FTransform transform;
				grid->HISMGround->GetInstanceTransform(tile.Instance, transform);

				FVector loc = transform.GetLocation() + FVector(0.0f, 0.0f, z);

				if (chosenLocations.Contains(loc))
					continue;

				chosenLocations.Add(loc);
			}
		}
	}

	return chosenLocations;
}

void ADiplosimGameModeBase::SpawnEnemies()
{
	FWaveStruct waveStruct;

	WavesData.Add(waveStruct);

	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	TArray<FVector> spawnLocations = PickSpawnPoints();

	LastLocation = FVector(0.0f, 0.0f, 0.0f);

	int32 num = citizens.Num() / 3;

	for (int32 i = 0; i < num; i++) {
		FTimerHandle locationTimer;
		GetWorld()->GetTimerManager().SetTimer(locationTimer, FTimerDelegate::CreateUObject(this, &ADiplosimGameModeBase::SpawnAtValidLocation, spawnLocations), 0.1f, false);
	}

	if (WavesData.Num() > 5) {
		WavesData.RemoveAt(0);
	}
}

void ADiplosimGameModeBase::SpawnAtValidLocation(TArray<FVector> SpawnLocations)
{
	int32 index = FMath::RandRange(0, SpawnLocations.Num() - 1);

	if (SpawnLocations[index] == LastLocation) {
		SpawnAtValidLocation(SpawnLocations);
		
		return;
	}

	LastLocation = SpawnLocations[index];

	AEnemy* enemy = GetWorld()->SpawnActor<AEnemy>(EnemyClass, SpawnLocations[index], FRotator(0, 0, 0));

	enemy->MoveToBroch();
}

int32 ADiplosimGameModeBase::GetRandomTime()
{
	int32 time = FMath::RandRange(earliestSpawnTime, latestSpawnTime);

	return time;
}

void ADiplosimGameModeBase::SetWaveTimer()
{
	TArray<AActor*> enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), enemies);

	for (int32 i = (enemies.Num() - 1); i > -1; i--) {
		AActor* enemy = enemies[i];

		UHealthComponent* healthComp = enemy->GetComponentByClass<UHealthComponent>();

		if (healthComp->Health > 0)
			continue;

		enemies.Remove(enemy);
	}

	if (!enemies.IsEmpty())
		return;

	int32 time = GetRandomTime();

	FTimerHandle spawnTimer;
	GetWorld()->GetTimerManager().SetTimer(spawnTimer, this, &ADiplosimGameModeBase::SpawnEnemies, time, false);

	SaveToFile();
}

void ADiplosimGameModeBase::SaveToFile()
{
	// Saving wave data to file for dissertation
	if (WavesData.IsEmpty())
		return;

	FString FilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + TEXT("/WaveLog.txt");

	FString FileContent;

	int32 count = 1;

	for (FWaveStruct wave : WavesData) {
		FString waveNum = FString::FromInt(count);

		FileContent += "Wave " + waveNum + " \n \n";

		FileContent += "Spawn Location \n" + wave.SpawnLocation.ToString() + " \n \n";

		FileContent += "Number of kills \n" + FString::FromInt(wave.NumKilled) + " \n \n";

		FileContent += "Number that died to an actor \n";

		TArray<FString> killers = wave.DiedTo;

		while (!killers.IsEmpty()) {
			FString name = killers[0];

			int32 tally = 0;

			for (int32 i = (killers.Num() - 1); i > -1; i--) {
				if (name != killers[i])
					continue;

				tally++;

				killers.RemoveAt(i);
			}

			FileContent += name + ": " + FString::FromInt(tally) + "\n";
		}

		FileContent += "\n \n";

		count++;
	}

	FFileHelper::SaveStringToFile(FileContent, *FilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get());
}