#include "DiplosimGameModeBase.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "Map/Grid.h"
#include "Buildings/Broch.h"

ADiplosimGameModeBase::ADiplosimGameModeBase()
{
	earliestSpawnTime = 900;
	latestSpawnTime = 1800;
}

TArray<FVector> ADiplosimGameModeBase::GetSpawnPoints(bool bCheckLength, bool bCheckSeaAdjacency)
{
	TArray<AActor*> brochs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABroch::StaticClass(), brochs);

	UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(brochs[0]->GetComponentByClass(UStaticMeshComponent::StaticClass()));

	TArray<AActor*> grids;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrid::StaticClass(), grids);

	AGrid* grid = Cast<AGrid>(grids[0]);

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

	TArray<FVector> locations;

	for (FTileStruct tile : grid->Storage) {
		if (tile.Choice == grid->HISMWater)
			continue;

		if (bCheckSeaAdjacency) {
			TArray<FTileStruct> tiles;

			tiles.Add(grid->Storage[(tile.X - 1) + (tile.Y * grid->Size)]);
			tiles.Add(grid->Storage[tile.X + ((tile.Y - 1) * grid->Size)]);
			tiles.Add(grid->Storage[(tile.X + 1) + (tile.Y * grid->Size)]);
			tiles.Add(grid->Storage[tile.X + ((tile.Y + 1) * grid->Size)]);

		
			bool bAdjacentToSea = false;

			for (FTileStruct t : tiles) {
				if (t.bIsSea) {
					bAdjacentToSea = true;

					break;
				}
			}

			if (!bAdjacentToSea)
				continue;
		}

		float z = tile.Choice->GetStaticMesh()->GetBoundingBox().GetSize().Z;

		FVector location = FVector(100.0f * tile.X - (100.0f * (grid->Size / 2)), 100.0f * tile.Y - (100.0f * (grid->Size / 2)), z);

		FPathFindingQuery query(this, *NavData, location, comp->GetSocketLocation("Entrance"));

		bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

		if (!path)
			continue;

		if (bCheckLength) {
			FVector::FReal outLength;
			NavData->CalcPathLength(location, comp->GetSocketLocation("Entrance"), outLength);

			if (outLength < 3000.0f)
				continue;
		}

		locations.Add(location);
	}

	return locations;
}

void ADiplosimGameModeBase::SpawnEnemies()
{
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	TArray<FVector> validLocations = GetSpawnPoints(true, true);

	if (validLocations.IsEmpty()) {
		validLocations = GetSpawnPoints(true, false);
	}

	if (validLocations.IsEmpty()) {
		validLocations = GetSpawnPoints(false, false);
	}

	int32 index = FMath::RandRange(0, validLocations.Num() - 1);

	FVector loc = validLocations[index]; // Use GOAP to eliminate already used and failed locations.

	int32 num = citizens.Num() / 3;

	for (int32 i = 0; i < num; i++) {
		AEnemy* enemy = GetWorld()->SpawnActor<AEnemy>(EnemyClass, loc, FRotator(0, 0, 0));

		enemy->MoveToBroch();
	}

	SetWaveTimer();
}

int32 ADiplosimGameModeBase::GetRandomTime()
{
	int32 time = FMath::RandRange(earliestSpawnTime, latestSpawnTime);

	return time;
}

void ADiplosimGameModeBase::SetWaveTimer()
{
	int32 time = GetRandomTime();

	FTimerHandle spawnTimer;
	GetWorld()->GetTimerManager().SetTimer(spawnTimer, this, &ADiplosimGameModeBase::SpawnEnemies, time, false);
}