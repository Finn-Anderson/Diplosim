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
struct FWaveStruct
{
	GENERATED_USTRUCT_BODY()

	FVector SpawnLocation;

	TArray<FDiedToStruct> DiedTo;

	int32 NumSpawned;

	int32 NumKilled;

	FWaveStruct()
	{
		SpawnLocation = FVector(0.0f, 0.0f, 0.0f);
		DiedTo = {};
		NumSpawned = 0;
		NumKilled = 0;
	}

	void SetDiedTo(AActor* Attacker, bool bIsProjectile)
	{
		AActor* actor = Attacker;

		if (bIsProjectile)
			actor = Attacker->Owner;

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

	bool PathToBuilding(class AGrid* Grid, FVector Location, float Length, bool bCheckForBroch);

	TArray<FVector> GetSpawnPoints(class AGrid* Grid, float Length, bool bCheckForBroch);

	TArray<FVector> PickSpawnPoints();

	void SpawnEnemies();

	void SpawnAtValidLocation(TArray<FVector> SpawnLocations);

	int32 GetRandomTime();

	void SetWaveTimer();

	void SaveToFile();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		TSubclassOf<class AEnemy> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		int32 earliestSpawnTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		int32 latestSpawnTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class UNavAreaBase> NavAreaThreat;

	TArray<FWaveStruct> WavesData;

	class ABuilding* Broch;

	TArray<FVector> LastLocation;

	TArray<class ACitizen*> Threats;
};
