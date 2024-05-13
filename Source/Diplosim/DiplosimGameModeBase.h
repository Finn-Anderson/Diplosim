#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DiplosimGameModeBase.generated.h"

USTRUCT()
struct FDiedToStruct
{
	GENERATED_USTRUCT_BODY()

	TWeakObjectPtr<AActor> Actor;

	int32 Kills;

	FDiedToStruct()
	{
		Actor = nullptr;
		Kills = 0;
	}

	bool operator==(const FDiedToStruct& other) const
	{
		return (other.Actor == Actor);
	}
};

USTRUCT()
struct FThreatsStruct
{
	GENERATED_USTRUCT_BODY()

	TWeakObjectPtr<class ACitizen> Citizen;

	FThreatsStruct()
	{
		Citizen = nullptr;
	}

	bool operator==(const FThreatsStruct& other) const
	{
		return (other.Citizen == Citizen);
	}
};

USTRUCT()
struct FWaveStruct
{
	GENERATED_USTRUCT_BODY()

	FVector SpawnLocation;

	TArray<FDiedToStruct> DiedTo;

	TArray<FThreatsStruct> Threats;

	int32 NumSpawned;

	int32 NumKilled;

	int32 TotalEnemies;

	int32 DeferredEnemies;

	FWaveStruct()
	{
		SpawnLocation = FVector(0.0f, 0.0f, 0.0f);
		DiedTo = {};
		Threats = {};
		NumSpawned = 0;
		NumKilled = 0;
		TotalEnemies = 0;
		DeferredEnemies = 0;
	}

	void SetDiedTo(AActor* Attacker)
	{
		AActor* actor = Attacker;

		FDiedToStruct diedTo;
		diedTo.Actor = actor;
		diedTo.Kills = 1;

		if (DiedTo.Contains(diedTo)) {
			int32 index;
			DiedTo.Find(diedTo, index);

			DiedTo[index].Kills++;
		}
		else {
			DiedTo.Add(diedTo);
		}
	}

	bool operator==(const FWaveStruct& other) const
	{
		return (other.SpawnLocation == SpawnLocation) && (other.DiedTo == DiedTo) && (other.NumKilled == NumKilled);
	}
};

UCLASS()
class DIPLOSIM_API ADiplosimGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADiplosimGameModeBase();

protected:
	virtual void BeginPlay() override;

public:
	void EvaluateThreats();

	bool PathToBuilding(FVector Location, class UNavigationSystemV1* Nav, const class ANavigationData* NavData);

	TArray<FVector> GetSpawnPoints();

	void PickSpawnPoints();

	void FindSpawnsInArea(struct FTileStruct* Tile, FVector TileLocation, TArray<FVector> ValidTiles, int32 Iteration);

	void SpawnEnemies();

	void SpawnAtValidLocation();

	void SpawnEnemiesAsync();

	bool CheckEnemiesStatus();

	void SetWaveTimer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		TSubclassOf<class AEnemy> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		int32 earliestSpawnTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		int32 latestSpawnTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class UNavAreaBase> NavAreaThreat;

	TArray<FVector> SpawnLocations;

	FTimerHandle WaveTimer;

	TArray<FWaveStruct> WavesData;

	class ABuilding* Broch;

	TArray<AActor*> Buildings;

	TArray<AActor*> Citizens;

	class AGrid* Grid;

	TArray<FVector> LastLocation;
};
