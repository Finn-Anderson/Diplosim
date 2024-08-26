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
#include "Buildings/Misc/Broch.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Player/Camera.h"

ADiplosimGameModeBase::ADiplosimGameModeBase()
{
	Broch = nullptr;
	Grid = nullptr;
}

void ADiplosimGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UDiplosimUserSettings::GetDiplosimUserSettings()->GameMode = this;
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

	LastLocation.Empty();

	AsyncTask(ENamedThreads::GameThread, [this]() {
		for (FEnemiesStruct &enemyData : EnemiesData) {
			int32 num = FMath::Floor(enemyData.Tally / 200.0f);

			for (int32 i = 0; i < num; i++) {
				FTimerHandle locationTimer;
				GetWorld()->GetTimerManager().SetTimer(locationTimer, FTimerDelegate::CreateUObject(this, &ADiplosimGameModeBase::SpawnAtValidLocation, enemyData.Colour), 0.1f, false);
			}

			enemyData.Tally = enemyData.Tally % 200;
		}

		CheckEnemiesStatus();
	});
}

void ADiplosimGameModeBase::SpawnEnemiesAsync()
{
	if (!CheckEnemiesStatus())
		return;

	GetWorld()->GetTimerManager().ClearTimer(WaveTimer);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), Buildings);
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), Citizens);

	Async(EAsyncExecution::Thread, [this]() { SpawnEnemies(); });
}

void ADiplosimGameModeBase::SpawnAtValidLocation(FLinearColor Colour)
{
	int32 index = FMath::RandRange(0, SpawnLocations.Num() - 1);

	if (LastLocation.Contains(SpawnLocations[index])) {
		SpawnAtValidLocation(Colour);
		
		return;
	}

	LastLocation.Add(SpawnLocations[index]);

	if (LastLocation.Num() > 10)
		LastLocation.RemoveAt(0);

	AEnemy* enemy = GetWorld()->SpawnActor<AEnemy>(EnemyClass, SpawnLocations[index], FRotator(0, 0, 0));

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(enemy->GetMesh()->GetMaterial(0), this);
	material->SetVectorParameterValue("Colour", Colour);
	material->SetScalarParameterValue("Emissiveness", 3.0f);
	enemy->GetMesh()->SetMaterial(0, material);

	enemy->EffectComponent->SetVariableLinearColor("Colour", Colour);

	enemy->MoveToBroch();

	WavesData.Last().TotalEnemies++;
}

bool ADiplosimGameModeBase::CheckEnemiesStatus()
{
	if (!WavesData.IsEmpty()) {
		int32 tally = 0;

		for (FDiedToStruct death : WavesData.Last().DiedTo)
			tally += death.Kills; 

		if (tally != WavesData.Last().TotalEnemies)
			return false;
	}

	if (!UDiplosimUserSettings::GetDiplosimUserSettings()->GetSpawnEnemies())
		return false;

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

	GetWorld()->GetTimerManager().SetTimer(WaveTimer, this, &ADiplosimGameModeBase::SpawnEnemiesAsync, 1800.0f, false);
}

void ADiplosimGameModeBase::TallyEnemyData(TSubclassOf<class AResource> Resource, int32 Amount)
{
	for (FEnemiesStruct& enemyData : EnemiesData)
		if (enemyData.Resources.Contains(Resource))
			enemyData.Tally += Amount;
}