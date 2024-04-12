#include "DiplosimGameModeBase.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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

void ADiplosimGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	const FString file = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + TEXT("/MovementLog.txt");

	IFileManager& fileMgr = IFileManager::Get();
	fileMgr.Delete(*file);
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

	// THIS SECTION OF CODE IS FOR TESTING. THE ABOVE CODE IS THE PRODUCTION CODE
	for (AActor* actor : Citizens) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->Building.Employment == nullptr || !citizen->Building.Employment->IsA<AWall>())
			continue;

		FThreatsStruct threatStruct;
		threatStruct.Citizen = citizen;
		threatStruct.EmploymentName = citizen->Building.Employment->GetName();
		threatStruct.Location = citizen->Building.Employment->GetActorLocation();

		WavesData.Last().Threats.Add(threatStruct);

		// CODE FOR TESTING AVOIDANCE. REMOVE TO TEST TARGETING.
		citizen->AttackComponent->RangeComponent->SetCanEverAffectNavigation(true);
	}
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

		if (t->Level < 0 || t->Level == 7)
			return;

		FVector loc = Grid->GetTransform(t).GetLocation();

		if (SpawnLocations.Contains(loc) || !ValidTiles.Contains(loc))
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

	int32 num = TotalEnemies;
	int32 max = UDiplosimUserSettings::GetDiplosimUserSettings()->GetMaxEnemies();
		
	if (TotalEnemies > max) {
		num = max;
		DeferredEnemies = TotalEnemies - num;
	}

	PickSpawnPoints();

	if (SpawnLocations.IsEmpty()) {
		AsyncTask(ENamedThreads::GameThread, [this]() { SetWaveTimer(); });

		return;
	}

	EvaluateThreats();

	LastLocation.Empty();

	AsyncTask(ENamedThreads::GameThread, [this, num, bSpawnTrails]() {
		for (int32 i = 0; i < num; i++) {
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
	if (DeferredEnemies > 0) {
		SpawnAtValidLocation(false);

		DeferredEnemies--;

		return false;
	}

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

	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + TEXT("/WaveLog.txt");

	FString fileContent;

	for (FWaveStruct wave : WavesData) {
		int32 index = 0;
		WavesData.Find(wave, index);
		index++;

		FString waveNum = FString::FromInt(index);

		fileContent += "Wave " + waveNum + " \n \n";

		fileContent += "Spawn location \n" + wave.SpawnLocation.ToString() + " \n \n";

		fileContent += "Number of spawned enemies \n" + FString::FromInt(wave.NumSpawned) + " \n \n";

		fileContent += "Number of enemy kills \n" + FString::FromInt(wave.NumKilled) + " \n \n";

		fileContent += "Number that died to an actor \n";

		for (FDiedToStruct diedTo : wave.DiedTo) {
			fileContent += diedTo.Name + ": " + FString::FromInt(diedTo.Kills) + "\n";
		}

		fileContent += "Threat location(s) \n";

		for (FThreatsStruct threatStruct : WavesData.Last().Threats) {
			fileContent += threatStruct.EmploymentName + ": " + threatStruct.Location.ToString() + "\n";
		}

		fileContent += "\n \n";
	}

	FFileHelper::SaveStringToFile(fileContent, *filePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get());
}
