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

	UPROPERTY()
		int32 Spawned;

	FEnemiesStruct()
	{
		Tally = 0;
		Spawned = 0;
		Colour = FLinearColor(1.0f, 1.0f, 1.0f);
	}
};

USTRUCT(BlueprintType)
struct FDiedToStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TWeakObjectPtr<AActor> Actor;

	UPROPERTY()
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
struct FWaveStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FVector> SpawnLocations;

	UPROPERTY()
		TArray<FDiedToStruct> DiedTo;

	UPROPERTY()
		TArray<TWeakObjectPtr<AActor>> Threats;

	UPROPERTY()
		int32 NumKilled;

	UPROPERTY()
		TArray<FEnemiesStruct> EnemiesData;

	FWaveStruct()
	{
		SpawnLocations = {};
		DiedTo = {};
		Threats = {};
		NumKilled = 0;
		EnemiesData = {};
	}

	void SetDiedTo(AActor* Attacker)
	{
		FDiedToStruct diedTo;
		diedTo.Actor = Attacker;
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
		return (other.SpawnLocations == SpawnLocations && other.DiedTo == DiedTo && other.NumKilled == NumKilled);
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
	void Tick(float DeltaTime) override;

	UFUNCTION()
		void SpawnAllEnemies();

	bool bSpawnedAllEnemies();

	UFUNCTION()
		void StartRaid();

	bool CheckEnemiesStatus();

	void SetWaveTimer();

	void TallyEnemyData(TSubclassOf<class AResource> Resource, int32 Amount);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		TSubclassOf<class AEnemy> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class UNavAreaBase> NavAreaThreat;

	UPROPERTY()
		TArray<class AAI*> Enemies;

	UPROPERTY()
		FTimerHandle WaveTimer;

	UPROPERTY()
		TArray<FWaveStruct> WavesData;

	UPROPERTY()
		class AGrid* Grid;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		TArray<FEnemiesStruct> EnemiesData;

	UPROPERTY()
		bool bOngoingRaid;

	UPROPERTY()
		float TargetOpacity;

private:
	void EvaluateThreats();

	bool PathToBuilding(FVector Location, class UNavigationSystemV1* Nav, const class ANavigationData* NavData);

	TArray<FVector> GetSpawnPoints();

	TArray<FVector> PickSpawnPoints();

	TArray<FVector> GetValidLocations(class UHierarchicalInstancedStaticMeshComponent* HISMComponent, TArray<int32> Instances, TArray<FVector> ValidTiles);

	void ShowRaidCrystal(bool bShow, FVector Location = FVector(0.0f, 0.0f, -1000.0f));

	void SetRaidInformation();

	void SpawnAtValidLocation(TArray<FVector> spawnLocations, FLinearColor Colour, int32* Spawned);

	int32 GetTotalSpawnedEnemies();
};
