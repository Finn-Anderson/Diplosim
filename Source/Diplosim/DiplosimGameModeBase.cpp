#include "DiplosimGameModeBase.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NavigationPath.h"

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

		FThreatsStruct threatStruct;
		threatStruct.Citizen = citizen;
		threatStruct.EmploymentName = citizen->Building.Employment->GetName();
		threatStruct.Location = citizen->Building.Employment->GetActorLocation();

		WavesData.Last().Threats.Add(threatStruct);

		int32 chance = FMath::RandRange(1, 30);
		chance -= threat.Kills;

		if (chance > 15)
			continue;

		citizen->AttackComponent->RangeComponent->SetCanEverAffectNavigation(true);
	}

	// CODE FOR TESTING AVOIDANCE
	/*TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	for (AActor* actor : citizens) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->Building.Employment == nullptr || !citizen->Building.Employment->IsA<AWall>())
			continue;

		citizen->AttackComponent->RangeComponent->SetCanEverAffectNavigation(true);
	}*/
}

bool ADiplosimGameModeBase::PathToBuilding(FVector Location, UNavigationSystemV1* Nav, const ANavigationData* NavData)
{
	double outLength = FVector::Dist(Location, Broch->GetActorLocation());

	if (outLength < 2000.0f)
		return false;

	for (AActor* actor : Buildings) {
		FVector loc = actor->GetActorLocation();
		loc.Y -= Cast<ABuilding>(actor)->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Y / 2;

		FPathFindingQuery query(NULL, *NavData, Location, loc);

		bool path = Nav->TestPathSync(query, EPathFindingMode::Hierarchical);

		if (path)
			return true;
	}

	return false;
}

TArray<FVector> ADiplosimGameModeBase::GetSpawnPoints()
{
	TArray<FVector> validTiles;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	float z = Grid->HISMGround->GetStaticMesh()->GetBoundingBox().GetSize().Z;

	for (TArray<FTileStruct>& row : Grid->Storage) {
		for (FTileStruct& tile : row) {
			if (tile.Level < 0)
				continue;

			FTransform transform;
			Grid->HISMGround->GetInstanceTransform(tile.Instance, transform);

			FVector location = transform.GetLocation() + FVector(0.0f, 0.0f, z);
			location.Z = FMath::RoundHalfFromZero(location.Z);

			if (!PathToBuilding(location, nav, navData))
				continue;

			validTiles.Add(location);
		}
	}

	return validTiles;
}

void ADiplosimGameModeBase::PickSpawnPoints()
{
	TArray<FVector> validTiles;
	TArray<FVector> chosenLocations;

	int32 num = -1;

	if (WavesData.Num() > 5)
		num = WavesData.Num() - 6;

	for (int32 i = WavesData.Num() - 1; i > num; i--) {
		FWaveStruct wave = WavesData[i];

		if (wave == WavesData.Last())
			continue;

		if (wave.DiedTo.Num() * 0.66f > wave.NumKilled) {
			validTiles.RemoveSingle(wave.SpawnLocation);
		}
		else {
			validTiles.Add(wave.SpawnLocation);
		}
	}

	if (validTiles.IsEmpty())
		validTiles = GetSpawnPoints();

	if (validTiles.IsEmpty())
		return;

	auto index = Async(EAsyncExecution::TaskGraph, [validTiles]() { return FMath::RandRange(0, validTiles.Num() - 1); });

	FWaveStruct waveStruct;

	WavesData.Add(waveStruct);

	WavesData.Last().SpawnLocation = validTiles[index.Get()];

	FTileStruct* chosenTile = &Grid->Storage[validTiles[index.Get()].X / 100 + Grid->Storage.Num() / 2][validTiles[index.Get()].Y / 100 + Grid->Storage.Num() / 2];

	int32 z = Grid->HISMGround->GetStaticMesh()->GetBoundingBox().GetSize().Z;

	FindSpawnsInArea(z, chosenTile, WavesData.Last().SpawnLocation, validTiles, 0);
}

void ADiplosimGameModeBase::FindSpawnsInArea(int32 Z, FTileStruct* Tile, FVector TileLocation, TArray<FVector> ValidTiles, int32 Iteration)
{
	SpawnLocations.Add(TileLocation);

	Iteration++;

	if (Iteration == 10)
		return;

	for (auto& element : Tile->AdjacentTiles) {
		FTileStruct* t = element.Value;

		FTransform transform;
		Grid->HISMGround->GetInstanceTransform(t->Instance, transform);

		FVector loc = transform.GetLocation() + FVector(0.0f, 0.0f, Z);

		if (t->Level < 0 || SpawnLocations.Contains(loc) || !ValidTiles.Contains(loc))
			continue;

		FindSpawnsInArea(Z, t, loc, ValidTiles, Iteration);
	}
}

void ADiplosimGameModeBase::SpawnEnemies(bool bSpawnTrails)
{
	SpawnLocations.Empty();

	TotalEnemies = 0;

	for (AActor* actor : Citizens) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->BioStruct.Age < 18)
			continue;

		TotalEnemies++;
	}

	TotalEnemies /= 3;

	PickSpawnPoints();

	if (SpawnLocations.IsEmpty()) {
		AsyncTask(ENamedThreads::GameThread, [this]() { SetWaveTimer(); });

		return;
	}

	EvaluateThreats();

	LastLocation.Empty();

	AsyncTask(ENamedThreads::GameThread, [this, bSpawnTrails]() {
		for (int32 i = 0; i < TotalEnemies; i++) {
			FTimerHandle locationTimer;
			GetWorld()->GetTimerManager().SetTimer(locationTimer, FTimerDelegate::CreateUObject(this, &ADiplosimGameModeBase::SpawnAtValidLocation, bSpawnTrails), 0.1f, false);
		}
	});
}

void ADiplosimGameModeBase::SpawnEnemiesAsync(bool bSpawnTrails)
{
	if (!CheckEnemiesStatus())
		return;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), Buildings);
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), Citizens);

	Async(EAsyncExecution::Thread, [this, bSpawnTrails]() { SpawnEnemies(bSpawnTrails); });
}

void ADiplosimGameModeBase::SpawnAtValidLocation(bool bSpawnTrails)
{
	int32 index = FMath::RandRange(0, SpawnLocations.Num() - 1);

	if (LastLocation.Contains(SpawnLocations[index])) {
		SpawnAtValidLocation(bSpawnTrails);
		
		return;
	}

	LastLocation.Add(SpawnLocations[index]);

	if (LastLocation.Num() > 10)
		LastLocation.RemoveAt(0);

	AEnemy* enemy = GetWorld()->SpawnActor<AEnemy>(EnemyClass, SpawnLocations[index], FRotator(0, 0, 0));

	if (bSpawnTrails) {
		UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailSystem, enemy->GetMesh(), NAME_None, FVector(0.0f), FRotator(0.0f), EAttachLocation::KeepRelativeOffset, true);

		float r = FMath::FRandRange(0.0f, 1.0f);
		float g = FMath::FRandRange(0.0f, 1.0f);
		float b = FMath::FRandRange(0.0f, 1.0f);

		NiagaraComp->SetVariableLinearColor(TEXT("Color"), FLinearColor(r, g, b));
	}

	enemy->MoveToBroch();

	WavesData.Last().NumSpawned++;
}

bool ADiplosimGameModeBase::CheckEnemiesStatus()
{
	if (!WavesData.IsEmpty() && WavesData.Last().DiedTo.Num() != TotalEnemies)
		return false;

	if (!GetWorld()->GetTimerManager().IsTimerActive(WaveTimer))
		SetWaveTimer();
	else
		GetWorld()->GetTimerManager().ClearTimer(WaveTimer);

	return true;
}

int32 ADiplosimGameModeBase::GetRandomTime()
{
	int32 time = FMath::RandRange(earliestSpawnTime, latestSpawnTime);

	return time;
}

void ADiplosimGameModeBase::SetWaveTimer()
{
	if (!WavesData.IsEmpty()) {
		for (FThreatsStruct threatStruct : WavesData.Last().Threats) {
			if (!threatStruct.Citizen->AttackComponent->RangeComponent->CanEverAffectNavigation())
				continue;

			threatStruct.Citizen->AttackComponent->RangeComponent->SetCanEverAffectNavigation(false);
		}
	}

	int32 time = GetRandomTime();

	GetWorld()->GetTimerManager().SetTimer(WaveTimer, FTimerDelegate::CreateUObject(this, &ADiplosimGameModeBase::SpawnEnemiesAsync, false), time, false);

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

		FileContent += "Spawn location \n" + wave.SpawnLocation.ToString() + " \n \n";

		FileContent += "Number of spawned enemies \n" + FString::FromInt(wave.NumSpawned) + " \n \n";

		FileContent += "Number of enemy kills \n" + FString::FromInt(wave.NumKilled) + " \n \n";

		FileContent += "Number that died to an actor \n";

		for (FDiedToStruct diedTo : wave.DiedTo) {
			FileContent += diedTo.Name + ": " + FString::FromInt(diedTo.Kills) + "\n";
		}

		FileContent += "Threat location(s) \n";

		for (FThreatsStruct threatStruct : WavesData.Last().Threats) {
			FileContent += threatStruct.EmploymentName + ": " + threatStruct.Location.ToString() + "\n";
		}

		FileContent += "\n \n";
	}

	FFileHelper::SaveStringToFile(FileContent, *FilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get());
}
