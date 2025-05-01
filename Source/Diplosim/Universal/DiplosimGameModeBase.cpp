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
	PrimaryActorTick.bCanEverTick = true;

	TargetOpacity = 0.0f;
	
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

void ADiplosimGameModeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UMaterialInstanceDynamic* material = Cast<UMaterialInstanceDynamic>(Camera->Grid->CrystalMesh->GetMaterial(0));

	float opacity = 0.0f;
	FHashedMaterialParameterInfo info;
	info.Name = FScriptName("Opacity");
	material->GetScalarParameterValue(info, opacity);

	if (TargetOpacity == 1.0f)
		opacity = FMath::Clamp(opacity + 0.005f, 0.0f, 1.0f);
	else
		opacity = FMath::Clamp(opacity - 0.005f, 0.0f, 1.0f);

	material->SetScalarParameterValue("Opacity", opacity);

	if (opacity == 0.0f || opacity == 1.0f)
		SetActorTickEnabled(false);
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
			if (!diedTo.Actor->IsValidLowLevelFast() || diedTo.Actor->IsA<ACitizen>())
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

		if (chance > 15 || Cast<AWall>(threat.Actor)->GetOccupied().IsEmpty())
			continue;

		AWall* wall = Cast<AWall>(threat.Actor);

		wall->SetRange();
		wall->RangeComponent->SetCanEverAffectNavigation(true);
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
		transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, 100.0f));

		if (ValidTiles.Contains(transform.GetLocation()))
			spawnLocations.Add(transform.GetLocation());
	}

	return spawnLocations;
}

void ADiplosimGameModeBase::ShowRaidCrystal(bool bShow, FVector Location)
{
	SetActorTickEnabled(true);

	if (bShow)
		TargetOpacity = 1.0f;
	else
		TargetOpacity = 0.0f;

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

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(Camera->Grid->CrystalMesh->GetMaterial(0), this);
	material->SetScalarParameterValue("Opacity", 0.0f);
	material->SetVectorParameterValue("Colour", FLinearColor(r, g, b));
	material->SetScalarParameterValue("Emissiveness", count / 10.0f);

	Camera->Grid->CrystalMesh->SetMaterial(0, material);

	Camera->DisplayEvent("Upcoming", "Raid");
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

	WavesData.Last().SpawnLocation = spawnLocations[0];

	EvaluateThreats();

	bOngoingRaid = true;

	AsyncTask(ENamedThreads::GameThread, [this, spawnLocations]() { ShowRaidCrystal(true, WavesData.Last().SpawnLocation + FVector(0.0f, 0.0f, 500.0f)); });

	FTimerStruct timer; // 120
	timer.CreateTimer("SpawnEnemies", this, 5, FTimerDelegate::CreateUObject(this, &ADiplosimGameModeBase::SpawnAllEnemies, spawnLocations), false, true);
	Camera->CitizenManager->Timers.Add(timer);
}

void ADiplosimGameModeBase::SpawnAllEnemies(TArray<FVector> SpawnLocations)
{
	int32 count = 1;

	for (FEnemiesStruct &enemyData : EnemiesData) {
		int32 num = FMath::Floor(enemyData.Tally / 200.0f);

		for (int32 i = 0; i < num; i++) {
			FTimerHandle spawnTimer;
			GetWorld()->GetTimerManager().SetTimer(spawnTimer, FTimerDelegate::CreateUObject(this, &ADiplosimGameModeBase::SpawnAtValidLocation, SpawnLocations, enemyData.Colour), 0.1f * count, false);

			count++;
		}

		enemyData.Tally = enemyData.Tally % 200;
	}

	FTimerHandle crystalTimer;
	GetWorld()->GetTimerManager().SetTimer(crystalTimer, FTimerDelegate::CreateUObject(this, &ADiplosimGameModeBase::ShowRaidCrystal, false, FVector(0.0f, 0.0f, -1000.0f)), 0.1f * count, false);

	Camera->DisplayEvent("Event", "Raid");
}

void ADiplosimGameModeBase::SpawnAtValidLocation(TArray<FVector> spawnLocations, FLinearColor Colour)
{
	int32 index = FMath::RandRange(0, spawnLocations.Num() - 1);

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FNavLocation navLocation;
	nav->ProjectPointToNavigation(spawnLocations[index], navLocation, FVector(100.0f, 100.0f, 50.0f));

	FActorSpawnParameters params;
	params.bNoFail = true;

	AEnemy* enemy = GetWorld()->SpawnActor<AEnemy>(EnemyClass, navLocation.Location, FRotator(0.0f), params);

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(enemy->Mesh->GetMaterial(0), enemy);
	material->SetVectorParameterValue("Colour", Colour);
	enemy->Mesh->SetMaterial(0, material);

	enemy->Colour = Colour;

	Camera->CitizenManager->Enemies.Add(enemy);

	enemy->MoveToBroch();

	WavesData.Last().TotalEnemies++;
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

	Camera->CitizenManager->RemoveTimer("WaveTimer", this);

	SetRaidInformation();
}

bool ADiplosimGameModeBase::CheckEnemiesStatus()
{
	if (!UDiplosimUserSettings::GetDiplosimUserSettings()->GetSpawnEnemies() || Camera->CitizenManager->FindTimer("SpawnEnemies", this) != nullptr)
		return false;
	
	if (bOngoingRaid) {
		int32 tally = 0;

		for (FDiedToStruct death : WavesData.Last().DiedTo)
			tally += death.Kills; 

		if (tally != WavesData.Last().TotalEnemies)
			return false;

		bOngoingRaid = false;
	}

	return true;
}

void ADiplosimGameModeBase::SetWaveTimer()
{
	if (!WavesData.IsEmpty()) {
		for (FThreatsStruct threat : WavesData.Last().Threats) {
			AWall* wall = Cast<AWall>(threat.Actor);

			if (!wall->RangeComponent->CanEverAffectNavigation())
				continue;

			wall->RangeComponent->SetCanEverAffectNavigation(false);
		}
	}

	FTimerStruct timer;
	timer.CreateTimer("WaveTimer", this, 1680, FTimerDelegate::CreateUObject(this, &ADiplosimGameModeBase::StartRaid), true, false);
	Camera->CitizenManager->Timers.Add(timer);
}

void ADiplosimGameModeBase::TallyEnemyData(TSubclassOf<class AResource> Resource, int32 Amount)
{
	for (FEnemiesStruct& enemyData : EnemiesData)
		if (enemyData.Resources.Contains(Resource))
			enemyData.Tally += Amount;
}