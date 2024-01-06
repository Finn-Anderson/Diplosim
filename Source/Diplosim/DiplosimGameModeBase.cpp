#include "DiplosimGameModeBase.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NavigationSystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/AttackComponent.h"
#include "Map/Grid.h"
#include "Buildings/Broch.h"
#include "Buildings/Wall.h"
#include "HealthComponent.h"

ADiplosimGameModeBase::ADiplosimGameModeBase()
{
	Broch = nullptr;

	earliestSpawnTime = 900;
	latestSpawnTime = 1800;
}

void ADiplosimGameModeBase::EvaluateThreats()
{
	int32 num = -1;

	if (WavesData.Num() > 5)
		num = WavesData.Num() - 6;

	TArray<FDiedToStruct> threats;

	for (int32 i = WavesData.Num() - 1; i > num; i--) {
		FWaveStruct wave = WavesData[i];

		for (FDiedToStruct diedTo : wave.DiedTo) {
			if (diedTo.Actor == nullptr || !diedTo.Actor->IsValidLowLevelFast())
				continue;

			if (threats.Contains(diedTo)) {
				int32 index;
				threats.Find(diedTo, index);

				threats[index].Kills += diedTo.Kills;
			}
			else {
				threats.Add(diedTo);
			}
		}
	}

	for (FDiedToStruct threat : threats) {
		if (threat.Kills < 10)
			continue;

		ACitizen* citizen = Cast<ACitizen>(threat.Actor);

		if (!citizen->Building.Employment->IsA<AWall>())
			continue;

		// Setup avoid/target code;
	}
}

bool ADiplosimGameModeBase::PathToBuilding(class AGrid* Grid, FVector Location, float Length, bool bCheckForBroch)
{
	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

	TArray<AActor*> buildings;

	if (bCheckForBroch) {
		buildings.Add(Broch);
	}
	else {
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), buildings);
	}

	for (AActor* actor : buildings) {
		ABuilding* building = Cast<ABuilding>(actor);

		FVector loc = building->GetActorLocation();

		FPathFindingQuery query(this, *NavData, Location, loc);

		bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

		if (!path)
			continue;

		double outLength;
		NavData->CalcPathLength(Location, loc, outLength);

		if (outLength < Length)
			continue;

		return true;
	}

	return false;
}

TArray<FVector> ADiplosimGameModeBase::GetSpawnPoints(class AGrid* Grid, float Length, bool bCheckForBroch)
{
	TArray<FVector> validTiles;

	for (FTileStruct tile : Grid->Storage) {
		if (tile.Level < 0)
			continue;

		FTransform transform;
		Grid->HISMGround->GetInstanceTransform(tile.Instance, transform);

		float z = Grid->HISMGround->GetStaticMesh()->GetBoundingBox().GetSize().Z;

		FVector location = transform.GetLocation() + FVector(0.0f, 0.0f, z);

		if (!PathToBuilding(Grid, location, Length, bCheckForBroch))
			continue;

		validTiles.Add(location);

		validTiles.Last().Z = FMath::RoundHalfFromZero(validTiles.Last().Z);
	}

	if (validTiles.IsEmpty()) {
		validTiles = GetSpawnPoints(Grid, Length, !bCheckForBroch);
	}

	if (validTiles.IsEmpty()) {
		validTiles = GetSpawnPoints(Grid, 500.0f, !bCheckForBroch);
	}

	return validTiles;
}

TArray<FVector> ADiplosimGameModeBase::PickSpawnPoints()
{
	TArray<AActor*> grids;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrid::StaticClass(), grids);

	AGrid* grid = Cast<AGrid>(grids[0]);

	TArray<FVector> validTiles;

	int32 num = -1;

	if (WavesData.Num() > 5)
		num = WavesData.Num() - 6;

	for (int32 i = WavesData.Num() - 1; i > num; i--) {
		FWaveStruct wave = WavesData[i];

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
		validTiles = GetSpawnPoints(grid, 3000.0f, true);
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

			FTileStruct tile = grid->Storage[(chosenTile.X + x) + ((chosenTile.Y + y) * grid->Size)];

			if (tile.Level < 0)
				continue;

			FTransform transform;
			grid->HISMGround->GetInstanceTransform(tile.Instance, transform);

			FVector loc = transform.GetLocation() + FVector(0.0f, 0.0f, z);

			if (validTiles.Num() > 10 && !validTiles.Contains(loc))
				continue;

			chosenLocations.Add(loc);
		}
	}

	return chosenLocations;
}

void ADiplosimGameModeBase::SpawnEnemies()
{
	FWaveStruct waveStruct;

	WavesData.Add(waveStruct);

	EvaluateThreats();

	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	TArray<FVector> spawnLocations = PickSpawnPoints();

	LastLocation.Empty();

	int32 num = 0;

	for (AActor* actor : citizens) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->BioStruct.Age < 18)
			continue;

		num++;
	}

	 num /= 3;

	for (int32 i = 0; i < num; i++) {
		FTimerHandle locationTimer;
		GetWorld()->GetTimerManager().SetTimer(locationTimer, FTimerDelegate::CreateUObject(this, &ADiplosimGameModeBase::SpawnAtValidLocation, spawnLocations), 0.1f, false);
	}
}

void ADiplosimGameModeBase::SpawnAtValidLocation(TArray<FVector> SpawnLocations)
{
	int32 index = FMath::RandRange(0, SpawnLocations.Num() - 1);

	if (LastLocation.Contains(SpawnLocations[index])) {
		SpawnAtValidLocation(SpawnLocations);
		
		return;
	}

	LastLocation.Add(SpawnLocations[index]);

	if (LastLocation.Num() > 10)
		LastLocation.RemoveAt(0);

	AEnemy* enemy = GetWorld()->SpawnActor<AEnemy>(EnemyClass, SpawnLocations[index], FRotator(0, 0, 0));

	enemy->MoveToBroch();

	WavesData.Last().NumSpawned++;
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

	for (FWaveStruct wave : WavesData) {
		int32 index = 0;
		WavesData.Find(wave, index);
		index++;

		FString waveNum = FString::FromInt(index);

		FileContent += "Wave " + waveNum + " \n \n";

		FileContent += "Spawn Location \n" + wave.SpawnLocation.ToString() + " \n \n";

		FileContent += "Number of Spawned Enemies \n" + FString::FromInt(wave.NumSpawned) + " \n \n";

		FileContent += "Number of kills \n" + FString::FromInt(wave.NumKilled) + " \n \n";

		FileContent += "Number that died to an actor \n";

		for (FDiedToStruct diedTo : wave.DiedTo) {
			FileContent += diedTo.Name + ": " + FString::FromInt(diedTo.Kills) + "\n";
		}

		FileContent += "\n \n";
	}

	FFileHelper::SaveStringToFile(FileContent, *FilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get());
}
