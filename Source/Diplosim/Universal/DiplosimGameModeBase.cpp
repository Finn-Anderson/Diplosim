#include "DiplosimGameModeBase.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/AttackComponent.h"
#include "Map/Grid.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "DiplosimUserSettings.h"

ADiplosimGameModeBase::ADiplosimGameModeBase()
{
	Broch = nullptr;
	Grid = nullptr;
	bOngoingRaid = false;
}

void ADiplosimGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld()->GetMapName() != "Map")
		return;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	settings->GameMode = this;

	settings->LoadIniSettings();
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

		FThreatsStruct threatStruct;
		threatStruct.Actor = threat.Actor;

		WavesData.Last().Threats.Add(threatStruct);

		int32 chance = FMath::RandRange(1, 30);
		chance -= threat.Kills;

		if (chance > 15)
			continue;

		UAttackComponent* attackComp = threat.Actor->GetComponentByClass<UAttackComponent>();

		attackComp->RangeComponent->SetCanEverAffectNavigation(true);
	}
}

bool ADiplosimGameModeBase::PathToBuilding(FVector Location, UNavigationSystemV1* Nav, const ANavigationData* NavData)
{
	double outLength = FVector::Dist(Location, Broch->GetActorLocation());

	if (outLength < 3000.0f)
		return false;

	FNavLocation loc;
	Nav->ProjectPointToNavigation(Location, loc, FVector(400.0f, 400.0f, 20.0f));

	for (int32 i = Camera->CitizenManager->Buildings.Num() - 1; i > -1; i--) {
		ABuilding* building = Camera->CitizenManager->Buildings[i];

		FNavLocation buildingLoc;
		Nav->ProjectPointToNavigation(building->GetActorLocation(), buildingLoc, FVector(400.0f, 400.0f, 20.0f));

		FPathFindingQuery query(NULL, *NavData, loc.Location, buildingLoc.Location);

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

TArray<FVector> ADiplosimGameModeBase::PickSpawnPoints()
{
	TArray<FVector> validTiles;
	TArray<FVector> chosenLocations;

	int32 num = -1;

	if (WavesData.Num() > 5)
		num = WavesData.Num() - 6;

	for (int32 i = WavesData.Num() - 1; i > num; i--) {
		FWaveStruct wave = WavesData[i];

		if (wave.DiedTo.Num() * 0.66f < wave.NumKilled)
			validTiles.Add(wave.SpawnLocation);
		else
			validTiles.RemoveSingle(wave.SpawnLocation);
	}

	if (validTiles.IsEmpty())
		validTiles = GetSpawnPoints();

	if (validTiles.IsEmpty())
		return validTiles;

	TArray<FVector> spawnLocations;

	auto index = Async(EAsyncExecution::TaskGraph, [validTiles]() { return FMath::RandRange(0, validTiles.Num() - 1); });

	WavesData.Last().SpawnLocation = validTiles[index.Get()];
	spawnLocations.Add(validTiles[index.Get()]);

	FTileStruct* chosenTile = &Grid->Storage[validTiles[index.Get()].X / 100 + Grid->Storage.Num() / 2][validTiles[index.Get()].Y / 100 + Grid->Storage.Num() / 2];

	TArray<int32> instances = Grid->HISMFlatGround->GetInstancesOverlappingSphere(Grid->GetTransform(chosenTile).GetLocation(), 1000);
	spawnLocations.Append(GetValidLocations(Grid->HISMFlatGround, instances, validTiles));

	instances = Grid->HISMGround->GetInstancesOverlappingSphere(Grid->GetTransform(chosenTile).GetLocation(), 1000);
	spawnLocations.Append(GetValidLocations(Grid->HISMGround, instances, validTiles));

	return spawnLocations;
}

TArray<FVector> ADiplosimGameModeBase::GetValidLocations(UHierarchicalInstancedStaticMeshComponent* HISMComponent, TArray<int32> Instances, TArray<FVector> ValidTiles)
{
	TArray<FVector> spawnLocations;

	for (int32 inst : Instances) {
		FTransform transform;

		HISMComponent->GetInstanceTransform(inst, transform);

		if (ValidTiles.Contains(transform.GetLocation()))
			spawnLocations.Add(transform.GetLocation());
	}

	return spawnLocations;
}

void ADiplosimGameModeBase::SpawnEnemies()
{
	TArray<FVector> spawnLocations = PickSpawnPoints();

	if (spawnLocations.IsEmpty()) {
		SetWaveTimer();

		return;
	}
	
	FWaveStruct waveStruct;

	WavesData.Add(waveStruct);

	EvaluateThreats();

	bOngoingRaid = true;

	AsyncTask(ENamedThreads::GameThread, [this, spawnLocations]() {
		FTimerHandle spawnTimer;

		int32 count = 0;

		for (FEnemiesStruct &enemyData : EnemiesData) {
			int32 num = FMath::Floor(enemyData.Tally / 200.0f);

			for (int32 i = 0; i < num; i++) {
				GetWorld()->GetTimerManager().SetTimer(spawnTimer, FTimerDelegate::CreateUObject(this, &ADiplosimGameModeBase::SpawnAtValidLocation, spawnLocations, enemyData.Colour), 0.1f * count, false);

				count++;
			}

			enemyData.Tally = enemyData.Tally % 200;
		}

		for (ACitizen* citizen : Camera->CitizenManager->Citizens)
			citizen->AttackComponent->RangeComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		Camera->DisplayEvent("Event", "Raid");
	});
}

void ADiplosimGameModeBase::SpawnEnemiesAsync()
{
	if (!CheckEnemiesStatus())
		return;

	bool bEnemies = false;

	for (FEnemiesStruct& enemyData : EnemiesData) {
		int32 num = FMath::Floor(enemyData.Tally / 200.0f);

		if (num <= 0)
			continue;

		bEnemies = true;

		break;
	}

	if (!bEnemies)
		return;

	GetWorld()->GetTimerManager().ClearTimer(WaveTimer);

	Async(EAsyncExecution::Thread, [this]() { SpawnEnemies(); });
}

void ADiplosimGameModeBase::SpawnAtValidLocation(TArray<FVector> spawnLocations, FLinearColor Colour)
{
	int32 index = FMath::RandRange(0, spawnLocations.Num() - 1);

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FNavLocation navLocation;
	nav->ProjectPointToNavigation(spawnLocations[index], navLocation, FVector(200.0f, 200.0f, 200.0f));

	AEnemy* enemy = GetWorld()->SpawnActor<AEnemy>(EnemyClass, navLocation.Location, FRotator(0, 0, 0));

	if (!IsValid(enemy))
		return;

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(enemy->Mesh->GetMaterial(0), enemy);
	material->SetVectorParameterValue("Colour", Colour);
	enemy->Mesh->SetMaterial(0, material);

	enemy->Colour = Colour;

	enemy->MoveToBroch();

	WavesData.Last().TotalEnemies++;
}

bool ADiplosimGameModeBase::CheckEnemiesStatus()
{
	if (!UDiplosimUserSettings::GetDiplosimUserSettings()->GetSpawnEnemies())
		return false;
	
	if (bOngoingRaid) {
		int32 tally = 0;

		for (FDiedToStruct death : WavesData.Last().DiedTo)
			tally += death.Kills; 

		if (tally != WavesData.Last().TotalEnemies)
			return false;

		bOngoingRaid = false;

		if (Camera->CitizenManager->Rebels.IsEmpty())
			for (ACitizen* citizen : Camera->CitizenManager->Citizens)
				citizen->AttackComponent->RangeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	return true;
}

void ADiplosimGameModeBase::SetWaveTimer()
{
	if (!WavesData.IsEmpty()) {
		for (FThreatsStruct threatStruct : WavesData.Last().Threats) {
			UAttackComponent* attackComp = threatStruct.Actor->GetComponentByClass<UAttackComponent>();

			if (!attackComp->RangeComponent->CanEverAffectNavigation())
				continue;

			attackComp->RangeComponent->SetCanEverAffectNavigation(false);
		}
	}

	GetWorld()->GetTimerManager().SetTimer(WaveTimer, this, &ADiplosimGameModeBase::SpawnEnemiesAsync, 1800.0f, true);
}

void ADiplosimGameModeBase::TallyEnemyData(TSubclassOf<class AResource> Resource, int32 Amount)
{
	for (FEnemiesStruct& enemyData : EnemiesData)
		if (enemyData.Resources.Contains(Resource))
			enemyData.Tally += Amount;
}