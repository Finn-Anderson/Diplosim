#include "DiplosimGameModeBase.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "DiplosimUserSettings.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/AttackComponent.h"
#include "Map/Grid.h"
#include "Buildings/Broch.h"
#include "Buildings/Wall.h"

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

		WavesData.Last().Threats.Add(threatStruct);

		int32 chance = FMath::RandRange(1, 30);
		chance -= threat.Kills;

		if (chance > 15)
			continue;

		citizen->AttackComponent->RangeComponent->SetCanEverAffectNavigation(true);
	}
}

bool ADiplosimGameModeBase::PathToBuilding(FVector Location, UNavigationSystemV1* Nav, const ANavigationData* NavData)
{
	double outLength = FVector::Dist(Location, Broch->GetActorLocation());

	if (outLength < 2000.0f)
		return false;

	for (AActor* actor : Buildings) {
		FNavLocation loc;
		Nav->ProjectPointToNavigation(actor->GetActorLocation(), loc, FVector(200.0f, 200.0f, 10.0f));

		FPathFindingQuery query(NULL, *NavData, Location, loc.Location);

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

	for (TArray<FTileStruct>& row : Grid->Storage) {
		for (FTileStruct& tile : row) {
			if (tile.Level < 0)
				continue;

			FVector loc = Grid->GetTransform(&tile).GetLocation();

			if (!PathToBuilding(loc, nav, navData))
				continue;

			validTiles.Add(loc);
		}
	}

	return validTiles;
}

void ADiplosimGameModeBase::PickSpawnPoints()
{
	TArray<FVector> validTiles;
	TArray<FVector> chosenLocations;

	if (WavesData.Num() >= 2) {
		FWaveStruct wave = WavesData[WavesData.Num() - 2];

		if (wave.DiedTo.Num() * 0.66f < wave.NumKilled)
			return;
	}

	if (validTiles.IsEmpty())
		validTiles = GetSpawnPoints();

	if (validTiles.IsEmpty())
		return;

	SpawnLocations.Empty();

	auto index = Async(EAsyncExecution::TaskGraph, [validTiles]() { return FMath::RandRange(0, validTiles.Num() - 1); });

	WavesData.Last().SpawnLocation = validTiles[index.Get()];

	FTileStruct* chosenTile = &Grid->Storage[validTiles[index.Get()].X / 100 + Grid->Storage.Num() / 2][validTiles[index.Get()].Y / 100 + Grid->Storage.Num() / 2];

	FindSpawnsInArea(chosenTile, WavesData.Last().SpawnLocation, validTiles, 0);
}

void ADiplosimGameModeBase::FindSpawnsInArea(FTileStruct* Tile, FVector TileLocation, TArray<FVector> ValidTiles, int32 Iteration)
{
	SpawnLocations.Add(TileLocation);

	Iteration++;

	if (Iteration == 10)
		return;

	for (auto& element : Tile->AdjacentTiles) {
		FTileStruct* t = element.Value;

		if (t->Level < 0 || t->Level == 7)
			return;

		FVector loc = Grid->GetTransform(t).GetLocation();

		if (SpawnLocations.Contains(loc) || !ValidTiles.Contains(loc))
			continue;

		FindSpawnsInArea(t, loc, ValidTiles, Iteration);
	}
}

void ADiplosimGameModeBase::SpawnEnemies()
{
	FWaveStruct waveStruct;

	WavesData.Add(waveStruct);

	PickSpawnPoints();

	if (SpawnLocations.IsEmpty()) {
		WavesData.Remove(WavesData.Last());

		AsyncTask(ENamedThreads::GameThread, [this]() { SetWaveTimer(); });

		return;
	}

	EvaluateThreats();

	for (AActor* actor : Citizens) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->BioStruct.Age < 18)
			continue;

		WavesData.Last().TotalEnemies++;
	}

	WavesData.Last().TotalEnemies /= 3;

	int32 num = WavesData.Last().TotalEnemies;
	int32 max = UDiplosimUserSettings::GetDiplosimUserSettings()->GetMaxEnemies();
		
	if (num > max) {
		WavesData.Last().DeferredEnemies = num - max;
		num = max;
	}

	LastLocation.Empty();

	AsyncTask(ENamedThreads::GameThread, [this, num]() {
		for (int32 i = 0; i < num; i++) {
			FTimerHandle locationTimer;
			GetWorld()->GetTimerManager().SetTimer(locationTimer, this, &ADiplosimGameModeBase::SpawnAtValidLocation, 0.1f, false);
		}
	});
}

void ADiplosimGameModeBase::SpawnEnemiesAsync()
{
	if (!CheckEnemiesStatus())
		return;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), Buildings);
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), Citizens);

	Async(EAsyncExecution::Thread, [this]() { SpawnEnemies(); });
}

void ADiplosimGameModeBase::SpawnAtValidLocation()
{
	int32 index = FMath::RandRange(0, SpawnLocations.Num() - 1);

	if (LastLocation.Contains(SpawnLocations[index])) {
		SpawnAtValidLocation();
		
		return;
	}

	LastLocation.Add(SpawnLocations[index]);

	if (LastLocation.Num() > 10)
		LastLocation.RemoveAt(0);

	AEnemy* enemy = GetWorld()->SpawnActor<AEnemy>(EnemyClass, SpawnLocations[index], FRotator(0, 0, 0));

	enemy->MoveToBroch();

	WavesData.Last().NumSpawned++;
}

bool ADiplosimGameModeBase::CheckEnemiesStatus()
{
	if (!WavesData.IsEmpty()) {
		if (WavesData.Last().DeferredEnemies > 0) {
			SpawnAtValidLocation();

			WavesData.Last().DeferredEnemies--;

			return false;
		}

		int32 tally = 0;

		for (FDiedToStruct death : WavesData.Last().DiedTo)
			tally += death.Kills; 

		if (tally != WavesData.Last().TotalEnemies)
			return false;
	}

	if (!GetWorld()->GetTimerManager().IsTimerActive(WaveTimer))
		SetWaveTimer();
	else
		GetWorld()->GetTimerManager().ClearTimer(WaveTimer);

	return true;
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

	int32 time = FMath::RandRange(earliestSpawnTime, latestSpawnTime);

	GetWorld()->GetTimerManager().SetTimer(WaveTimer, this, &ADiplosimGameModeBase::SpawnEnemiesAsync, time, false);
}