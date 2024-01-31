#include "DiplosimGameModeBase.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
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

		if (citizen->Building.Employment == nullptr || !citizen->Building.Employment->IsA<AWall>())
			continue;

		Threats.Add(citizen);

		int32 chance = FMath::RandRange(1, 30);
		chance -= threat.Kills;

		if (chance > 15)
			continue;

		citizen->AttackComponent->RangeComponent->SetCanEverAffectNavigation(true);
	}

	// CODE FOR TESTING
	/*TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	for (AActor* actor : citizens) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->Building.Employment == nullptr || !citizen->Building.Employment->IsA<AWall>())
			continue;

		citizen->AttackComponent->RangeComponent->SetCanEverAffectNavigation(true);
	}*/
}

bool ADiplosimGameModeBase::PathToBuilding(FVector Location, UNavigationSystemV1* Nav, const ANavigationData* NavData, TArray<AActor*> Buildings)
{
	double outLength = FVector::Dist(Location, Broch->GetActorLocation());

	if (outLength < 2000.0f)
		return false;

	for (AActor* actor : Buildings) {
		FPathFindingQuery query(this, *NavData, Location, actor->GetActorLocation());

		bool path = Nav->TestPathSync(query, EPathFindingMode::Hierarchical);

		if (!path)
			continue;

		return true;
	}

	return false;
}

TArray<FVector> ADiplosimGameModeBase::GetSpawnPoints(AGrid* Grid, UNavigationSystemV1* Nav, const ANavigationData* NavData, TArray<AActor*> Buildings)
{
	TArray<FVector> validTiles;

	float z = Grid->HISMGround->GetStaticMesh()->GetBoundingBox().GetSize().Z;

	for (TArray<FTileStruct>& row : Grid->Storage) {
		for (FTileStruct& tile : row) {
			if (tile.Level < 0)
				continue;

			FTransform transform;
			Grid->HISMGround->GetInstanceTransform(tile.Instance, transform);

			FVector location = transform.GetLocation() + FVector(0.0f, 0.0f, z);

			if (!PathToBuilding(location, Nav, NavData, Buildings))
				continue;

			validTiles.Add(location);

			validTiles.Last().Z = FMath::RoundHalfFromZero(validTiles.Last().Z);
		}
	}

	return validTiles;
}

TArray<FVector> ADiplosimGameModeBase::PickSpawnPoints()
{
	TArray<AActor*> grids;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrid::StaticClass(), grids);

	AGrid* grid = Cast<AGrid>(grids[0]);

	TArray<FVector> validTiles;
	TArray<FVector> chosenLocations;

	int32 num = -1;

	if (WavesData.Num() > 5)
		num = WavesData.Num() - 6;

	for (int32 i = WavesData.Num() - 1; i > num; i--) {
		FWaveStruct wave = WavesData[i];

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
		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

		TArray<AActor*> buildings;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), buildings);

		validTiles = GetSpawnPoints(grid, nav, NavData, buildings);
	}

	if (validTiles.IsEmpty())
		return chosenLocations;

	int32 index = FMath::RandRange(0, validTiles.Num() - 1);

	FWaveStruct waveStruct;

	WavesData.Add(waveStruct);

	WavesData.Last().SpawnLocation = validTiles[index];

	FTileStruct* chosenTile = &grid->Storage[validTiles[index].X / 100 + grid->Storage.Num() / 2][validTiles[index].Y / 100 + grid->Storage.Num() / 2];

	int32 z = grid->HISMGround->GetStaticMesh()->GetBoundingBox().GetSize().Z;

	chosenLocations = FindSpawnsInArea(grid, z, chosenTile, validTiles, 0);

	return chosenLocations;
}

TArray<FVector> ADiplosimGameModeBase::FindSpawnsInArea(AGrid* Grid, int32 Z, FTileStruct* Tile, TArray<FVector> ValidTiles, int32 Iteration)
{
	TArray<FVector> locations;

	FTransform transform;
	Grid->HISMGround->GetInstanceTransform(Tile->Instance, transform);

	FVector loc = transform.GetLocation() + FVector(0.0f, 0.0f, Z);

	if (!ValidTiles.Contains(loc))
		return locations;

	locations.Add(loc);

	Iteration++;

	if (Iteration == 10)
		return locations;

	for (auto& element : Tile->AdjacentTiles) {
		FTileStruct* t = element.Value;

		if (t->Level < 0)
			continue;

		locations.Append(FindSpawnsInArea(Grid, Z, t, ValidTiles, Iteration));
	}

	return locations;
}

void ADiplosimGameModeBase::SpawnEnemies()
{
	EvaluateThreats();

	TArray<FVector> spawnLocations = PickSpawnPoints();

	if (spawnLocations.IsEmpty()) {
		SetWaveTimer();

		return;
	}

	LastLocation.Empty();

	int32 num = 0;

	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

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

	for (ACitizen* citizen : Threats) {
		if (!citizen->AttackComponent->RangeComponent->CanEverAffectNavigation())
			continue;

		citizen->AttackComponent->RangeComponent->SetCanEverAffectNavigation(false);
	}

	Threats.Empty();

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
