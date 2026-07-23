#include "Universal/DiplosimGameModeBase.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Misc/ScopeTryLock.h"

#include "AI/Enemy.h"
#include "AI/AISpawner.h"
#include "AI/Citizen/Citizen.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Buildings/Misc/Special/CloneLab.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Components/SaveGameComponent.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/PoliceManager.h"
#include "Universal/DiplosimUserSettings.h"
#include "DebugManager.h"

ADiplosimGameModeBase::ADiplosimGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(0.01f);

	TargetOpacity = 0.0f;
	
	Grid = nullptr;
}

void ADiplosimGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld()->GetMapName() != "Map")
		return;

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	settings->GameMode = this;
}

void ADiplosimGameModeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsValid(Camera) || Camera->CustomTimeDilation > 1.0f || DeltaTime > 1.0f)
		return;

	float oldOpacity = Grid->CrystalMesh->GetCustomPrimitiveData().Data[0];
	float increment = 0.005f;

	if (TargetOpacity != 1.0f)
		increment *= -1.0f;

	float newOpacity = FMath::Clamp(oldOpacity + increment, -0.01f, 1.0f);

	if (oldOpacity != newOpacity)
		Grid->CrystalMesh->SetCustomPrimitiveDataFloat(0, newOpacity);
	else
		SetActorTickEnabled(false);
}

bool ADiplosimGameModeBase::IsSnakeFaction(AActor* Actor)
{
	return Snakes.Contains(Actor) || SnakeSpawners.Contains(Actor);
}

bool ADiplosimGameModeBase::IsEnemy(AActor* Actor)
{
	return Enemies.Contains(Actor) || IsSnakeFaction(Actor);
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
			if (diedTo.Actor == nullptr || diedTo.Actor->IsA<ACitizen>())
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

		WavesData.Last().Threats.Add(threat.Actor);

		int32 chance = Async(EAsyncExecution::TaskGraphMainThread, [this]() { return Camera->Stream.RandRange(1, 30); }).Get();
		chance -= threat.Kills;

		if (chance > 15 || Cast<AWall>(threat.Actor)->GetOccupied().IsEmpty())
			continue;

		AWall* wall = Cast<AWall>(threat.Actor);

		wall->SetRange();
		wall->RangeComponent->SetCanEverAffectNavigation(true);
	}
}

bool ADiplosimGameModeBase::PathToBuilding(FVector Location, UNavigationSystemV1* Nav, const ANavigationData* NavData)
{
	FNavLocation loc;
	Nav->ProjectPointToNavigation(Location, loc, FVector(400.0f, 400.0f, 20.0f));

	bool bPath = false;

	for (FFactionStruct faction : Camera->ConquestManager->Factions) {
		for (ABuilding* building : faction.Buildings) {
			if (!IsValid(building) || building->HealthComponent == 0)
				continue;

			if (building->IsA<ABroch>() && FVector::Dist(Location, building->GetActorLocation()) < 3000.0f)
				return false;

			if (bPath)
				continue;

			FNavLocation buildingLoc;
			Nav->ProjectPointToNavigation(building->GetActorLocation(), buildingLoc, FVector(400.0f, 400.0f, 20.0f));

			FPathFindingQuery query(NULL, *NavData, loc.Location, buildingLoc.Location);

			bPath = Nav->TestPathSync(query, EPathFindingMode::Hierarchical);
		}
	}

	return bPath;
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
		int32 kills = 0;

		for (FDiedToStruct diedTo : wave.DiedTo)
			kills += diedTo.Kills;

		if (wave.NumKilled * 0.66f > kills)
			validTiles.Add(wave.SpawnLocations[0]);
		else
			validTiles.RemoveSingle(wave.SpawnLocations[0]);
	}

	if (validTiles.IsEmpty())
		validTiles = GetSpawnPoints();

	if (validTiles.IsEmpty())
		return validTiles;

	TArray<FVector> spawnLocations;
	FVector startLocation = validTiles[Camera->Stream.RandRange(0, validTiles.Num() - 1)];
	spawnLocations.Add(startLocation);

	TArray<int32> instances = Grid->HISMGround->GetInstancesOverlappingSphere(startLocation, 1000);
	spawnLocations.Append(GetValidLocations(Grid->HISMGround, instances, validTiles));

	return spawnLocations;
}

TArray<FVector> ADiplosimGameModeBase::GetValidLocations(UHierarchicalInstancedStaticMeshComponent* HISMComponent, TArray<int32> Instances, TArray<FVector> ValidTiles)
{
	TArray<FVector> spawnLocations;

	for (int32 inst : Instances) {
		FTransform transform;

		HISMComponent->GetInstanceTransform(inst, transform);
		transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, 100.0f));

		if (ValidTiles.Contains(transform.GetLocation()))
			spawnLocations.Add(transform.GetLocation());
	}

	return spawnLocations;
}

void ADiplosimGameModeBase::ShowRaidCrystal(bool bShow, FVector Location)
{
	SetActorTickEnabled(true);

	TargetOpacity = bShow ? 1.0f : 0.0f;

	if (Location != FVector::Zero())
		Camera->Grid->CrystalMesh->SetRelativeLocation(Location);

	if (!bShow)
		return;

	int32 count = 0;

	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;

	for (FEnemiesStruct& enemyData : EnemiesData) {
		int32 num = FMath::Floor(enemyData.Tally / 200.0f);

		r += enemyData.Colour.R * num;
		g += enemyData.Colour.G * num;
		b += enemyData.Colour.B * num;

		count += num;
	}

	r /= count;
	g /= count;
	b /= count;

	Camera->Grid->CrystalMesh->SetCustomPrimitiveDataFloat(0, 0.0f);
	Camera->Grid->CrystalMesh->SetCustomPrimitiveDataFloat(1, r);
	Camera->Grid->CrystalMesh->SetCustomPrimitiveDataFloat(2, g);
	Camera->Grid->CrystalMesh->SetCustomPrimitiveDataFloat(3, b);
	Camera->Grid->CrystalMesh->SetCustomPrimitiveDataFloat(5, count / 10.0f);

	Camera->ShowEvent("Upcoming", "Raid");
}

void ADiplosimGameModeBase::SetRaidInformation()
{
	TArray<FVector> spawnLocations = PickSpawnPoints();

	if (spawnLocations.IsEmpty()) {
		SetWaveTimer();

		return;
	}

	FWaveStruct waveStruct;
	WavesData.Add(waveStruct);

	WavesData.Last().SpawnLocations = spawnLocations;

	EvaluateThreats();

	Async(EAsyncExecution::TaskGraphMainTick, [this]() { ShowRaidCrystal(true, WavesData.Last().SpawnLocations[0] + FVector(0.0f, 0.0f, 500.0f)); });

	int32 time = Cast<UDebugManager>(Camera->PController->CheatManager)->bInstantEnemies ? 5 : 120;

	Camera->TimerManager->CreateTimer("SpawnEnemies", this, time, "SpawnAllEnemies", {}, false, true);
}

void ADiplosimGameModeBase::SpawnAllEnemies()
{
	Cast<UDebugManager>(Camera->PController->CheatManager)->bInstantEnemies = false;

	for (FFactionStruct& faction : Camera->ConquestManager->Factions)
		Camera->PoliceManager->CeaseAllInternalFighting(&faction);

	int32 count = 1;

	for (int32 i = 0; i < EnemiesData.Num(); i++) {
		int32 num = FMath::Floor(EnemiesData[i].Tally / 200.0f);

		for (int32 j = 0; j < num; j++) {
			TArray<FTimerParameterStruct> params;
			Camera->TimerManager->SetParameter(EnemiesData[i].Colour, params);
			Camera->TimerManager->CreateTimer("SpawnEnemy" + i + j, this, 0.1f * count, "SpawnAtValidLocation", params, false, true);

			count++;
		}

		EnemiesData[i].Tally = EnemiesData[i].Tally % 200;
	}

	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(false, params);
	Camera->TimerManager->SetParameter(FVector::Zero(), params);
	Camera->TimerManager->CreateTimer("ShowRaidCrystal", this, 0.1f * count, "ShowRaidCrystal", params, false, true);

	for (ASpecial* building : Grid->SpecialBuildings) {
		if (!IsValid(building) || !building->IsA<ACloneLab>())
			continue;

		Cast<ACloneLab>(building)->StartCloneLab();

		break;
	}

	Camera->ShowEvent("Event", "Raid");
}

void ADiplosimGameModeBase::SpawnAtValidLocation(FLinearColor Colour)
{
	int32 index = Camera->Stream.RandRange(0, WavesData.Last().SpawnLocations.Num() - 1);

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FNavLocation navLocation;
	nav->ProjectPointToNavigation(WavesData.Last().SpawnLocations[index], navLocation, FVector(100.0f, 100.0f, 50.0f));

	FActorSpawnParameters params;
	params.bNoFail = true;

	FTransform transform;
	transform.SetLocation(navLocation.Location);

	AEnemy* enemy = GetWorld()->SpawnActor<AEnemy>(EnemyClass, FVector::Zero(), FRotator(0.0f), params);
	enemy->Colour = Colour;

	Enemies.Add(enemy);
	Grid->AIVisualiser->AddInstance(enemy, Grid->AIVisualiser->HISMEnemy, transform);

	enemy->MoveToBroch();
}

void ADiplosimGameModeBase::StartRaid()
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

	Camera->TimerManager->RemoveTimer("WaveTimer", this);

	SetRaidInformation();
}

bool ADiplosimGameModeBase::CheckEnemiesStatus()
{
	return UDiplosimUserSettings::GetDiplosimUserSettings()->GetSpawnEnemies() && Grid->CrystalMesh->GetCustomPrimitiveData().Data[0] <= 0.0f && Enemies.IsEmpty();
}

void ADiplosimGameModeBase::TallyEnemyData(TSubclassOf<class AResource> Resource, int32 Amount)
{
	for (FEnemiesStruct& enemyData : EnemiesData) {
		if (!enemyData.Resources.Contains(Resource))
			continue;

		enemyData.Tally += (Amount < 200 ? Amount * enemyData.Weight : Amount);

		break;
	}
}

void ADiplosimGameModeBase::SetWaveTimer()
{
	if (!WavesData.IsEmpty()) {
		for (TWeakObjectPtr<AActor> threat : WavesData.Last().Threats) {
			AWall* wall = Cast<AWall>(threat);

			if (!wall->RangeComponent->CanEverAffectNavigation())
				continue;

			wall->RangeComponent->SetCanEverAffectNavigation(false);
		}
	}

	int32 timeToCompleteDay = Grid->AtmosphereComponent->GetTimeToCompleteDay();

	Camera->TimerManager->CreateTimer("WaveTimer", this, timeToCompleteDay * 2, "StartRaid", {}, true, false);
}

void ADiplosimGameModeBase::CheckWaveTimer()
{
	if (Camera->Start) {
		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Wave);

			return;
		}

		return;
	}

	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&WaveLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Wave);

			return;
		}

		FString time = "Imminent";
		float percentage = 0.0f;

		float waveTimer = Camera->TimerManager->GetElapsedTime("WaveTimer", this);
		int32 timeToCompleteDay = Grid->AtmosphereComponent->GetTimeToCompleteDay();

		int32 days = FMath::RoundToInt32(waveTimer / timeToCompleteDay);

		if (days >= 1)
			time = FString::FromInt(days) + (days == 1 ? " day" : " days");

		percentage = Camera->TimerManager->GetElapsedPercentage("WaveTimer", this);

		Async(EAsyncExecution::TaskGraphMainThread, [this, time, percentage]() { Camera->UpdateWaveUI(time, percentage); });
	});
}