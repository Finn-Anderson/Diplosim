#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DiplosimGameModeBase.generated.h"

UCLASS()
class DIPLOSIM_API ADiplosimGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADiplosimGameModeBase();

	TArray<FVector> GetSpawnPoints(bool bCheckLength);

	void SetSpawnPoints();

	void SpawnEnemies();

	int32 GetRandomTime();

	void SetWaveTimer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		TSubclassOf<class AEnemy> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		int32 earliestSpawnTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
		int32 latestSpawnTime;
};
