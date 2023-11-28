#include "DiplosimGameModeBase.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "Buildings/Broch.h"

ADiplosimGameModeBase::ADiplosimGameModeBase()
{
	earliestSpawnTime = 900;
	latestSpawnTime = 1800;
}

void ADiplosimGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	SetWaveTimer();
}

void ADiplosimGameModeBase::SpawnEnemies()
{
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	int32 num = citizens.Num() / 3;
	FVector loc = FVector(0.0f, 0.0f, 0.0f); // Figure out modular spawn positions. GOAP?

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