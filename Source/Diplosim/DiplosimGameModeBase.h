#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DiplosimGameModeBase.generated.h"

USTRUCT()
struct FWaveStruct
{
	GENERATED_USTRUCT_BODY()

	FVector SpawnLocation;

	TArray<FString> DiedTo;

	int32 NumKilled;

	FWaveStruct()
	{
		SpawnLocation = FVector(0.0f, 0.0f, 0.0f);
		DiedTo = {};
		NumKilled = 0;
	}
};

UCLASS()
class DIPLOSIM_API ADiplosimGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADiplosimGameModeBase();

	bool PathToBroch(class AGrid* Grid, struct FTileStruct tile, bool bCheckLength);

	TArray<FTileStruct> GetSpawnPoints(class AGrid* Grid, bool bCheckLength, bool bCheckSeaAdjacency);

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

	TArray<FWaveStruct> WavesData;

	class ABuilding* Broch;

	FVector LastLocation;
};
