#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "DebugManager.generated.h"

UCLASS()
class DIPLOSIM_API UDebugManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UDebugManager();

	UPROPERTY()
		bool bInstantBuildCheat;

	UPROPERTY()
		bool bInstantEnemies;

	UPROPERTY()
		bool bRain;

	UPROPERTY()
		bool bFight;

	UPROPERTY()
		int32 Happiness;

	UFUNCTION(Exec)
		void SpawnEnemies();

	UFUNCTION(Exec)
		void ChangeSeasonAffect(FString Season);

	UFUNCTION(Exec)
		void SetRain(bool bChance);

	UFUNCTION(Exec)
		void SetWindSpeed(int32 Speed);

	UFUNCTION(Exec)
		void AddEnemies(FString ResourceName, int32 Amount);

	UFUNCTION(Exec)
		void AddResearch(float Amount);

	UFUNCTION(Exec)
		void TurnOnInstantBuild(bool Value);

	UFUNCTION(Exec)
		void FillStorage();

	UFUNCTION(Exec)
		void SpawnCitizen(int32 Amount, int32 Age);

	UFUNCTION(Exec)
		void SetEvent(FString Type, FString Period, int32 Day, int32 StartHour, int32 EndHour, bool bRecurring, bool bFireFestival = false);

	UFUNCTION(Exec)
		void DamageActor(int32 Amount);

	UFUNCTION(Exec)
		void SetOnFire();

	UFUNCTION(Exec)
		void GiveProblem(bool bInjury);

	UFUNCTION(Exec)
		void CauseNaturalDisaster();

	UFUNCTION(Exec)
		void SetFight(bool bChance);

	UFUNCTION(Exec)
		void SetHappiness(int32 Value);
};
