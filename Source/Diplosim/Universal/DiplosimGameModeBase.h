#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DiplosimGameModeBase.generated.h"

USTRUCT(BlueprintType)
struct FEnemiesStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		TArray<TSubclassOf<class AResource>> Resources;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		int32 Tally;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		FLinearColor Colour;

	FEnemiesStruct()
	{
		Tally = 0;
		Colour = FLinearColor(1.0f, 1.0f, 1.0f);
	}
};

USTRUCT(BlueprintType)
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

USTRUCT(BlueprintType)
struct FThreatsStruct
{
	GENERATED_USTRUCT_BODY()

	TWeakObjectPtr<AActor> Actor;

	FThreatsStruct()
	{
		Actor = nullptr;
	}

	bool operator==(const FThreatsStruct& other) const
	{
		return (other.Actor == Actor);
	}
};

USTRUCT(BlueprintType)
struct FWaveStruct
{
	GENERATED_USTRUCT_BODY()

	FVector SpawnLocation;

	TArray<FDiedToStruct> DiedTo;

	TArray<FThreatsStruct> Threats;

	int32 NumKilled;

	int32 TotalEnemies;

	FWaveStruct()
	{
		SpawnLocation = FVector(0.0f, 0.0f, 0.0f);
		DiedTo = {};
		Threats = {};
		NumKilled = 0;
		TotalEnemies = 0;
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
	void BeginPlay() override;

public:
	void EvaluateThreats();

	bool PathToBuilding(FVector Location, class UNavigationSystemV1* Nav, const class ANavigationData* NavData);

	TArray<FVector> GetSpawnPoints(class UNavigationSystemV1* Nav, const class ANavigationData* NavData);

	TArray<FVector> PickSpawnPoints();

	TArray<FVector> GetValidLocations(class UHierarchicalInstancedStaticMeshComponent* HISMComponent, TArray<int32> Instances, class UNavigationSystemV1* Nav, const class ANavigationData* NavData);

	void SpawnEnemies();

	void SpawnAtValidLocation(TArray<FVector> spawnLocations, FLinearColor Colour);

	void SpawnEnemiesAsync();

	bool CheckEnemiesStatus();

	void SetWaveTimer();

	void TallyEnemyData(TSubclassOf<class AResource> Resource, int32 Amount);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		TSubclassOf<class AEnemy> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class UNavAreaBase> NavAreaThreat;

	UPROPERTY()
		FTimerHandle WaveTimer;

	UPROPERTY()
		TArray<FWaveStruct> WavesData;

	UPROPERTY()
		class ABuilding* Broch;

	UPROPERTY()
		class AGrid* Grid;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		TArray<FEnemiesStruct> EnemiesData;
};
