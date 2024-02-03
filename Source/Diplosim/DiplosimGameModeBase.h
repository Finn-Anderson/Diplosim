#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DiplosimGameModeBase.generated.h"

USTRUCT()
struct FDiedToStruct
{
	GENERATED_USTRUCT_BODY()

	TWeakObjectPtr<AActor> Actor;

	FString Name;

	int32 Kills;

	FDiedToStruct()
	{
		Actor = nullptr;
		Name = "";
		Kills = 0;
	}

	bool operator==(const FDiedToStruct& other) const
	{
		return (other.Actor == Actor) && (other.Name == Name);
	}
};

USTRUCT()
struct FThreatsStruct
{
	GENERATED_USTRUCT_BODY()

	TWeakObjectPtr<class ACitizen> Citizen;

	FString EmploymentName;

	FVector Location;

	FThreatsStruct()
	{
		Citizen = nullptr;
		EmploymentName = "";
		Location = FVector::Zero();
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

	FWaveStruct()
	{
		SpawnLocation = FVector(0.0f, 0.0f, 0.0f);
		DiedTo = {};
		NumSpawned = 0;
		NumKilled = 0;
		Threats = {};
	}

	void SetDiedTo(ACitizen* Attacker, bool bIsProjectile)
	{
		AActor* actor = Attacker;

		if (bIsProjectile)
			actor = Attacker->Building.Employment;

		FDiedToStruct diedTo;
		diedTo.Actor = actor;
		diedTo.Name = Attacker->GetName();
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

	void EvaluateThreats();

	bool PathToBuilding(FVector Location, class UNavigationSystemV1* Nav, const class ANavigationData* NavData, TArray<AActor*> Buildings);

	TArray<FVector> GetSpawnPoints(class AGrid* Grid);

	void PickSpawnPoints();

	void FindSpawnsInArea(AGrid* Grid, int32 Z, struct FTileStruct* Tile, FVector TileLocation, TArray<FVector> ValidTiles, int32 Iteration);

	void SpawnEnemies(bool bSpawnTrails = false);

	void SpawnAtValidLocation(bool bSpawnTrails);

	bool CheckEnemiesStatus();

	int32 GetRandomTime();

	void SetWaveTimer();

	void SaveToFile();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		TSubclassOf<class AEnemy> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		int32 earliestSpawnTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		int32 latestSpawnTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		class UNiagaraSystem* TrailSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class UNavAreaBase> NavAreaThreat;

	TArray<FVector> SpawnLocations;

	FTimerHandle WaveTimer;

	TArray<FWaveStruct> WavesData;

	class ABuilding* Broch;

	TArray<FVector> LastLocation;

	int32 TotalEnemies;
};
