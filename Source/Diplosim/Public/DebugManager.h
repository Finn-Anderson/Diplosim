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

	UFUNCTION(Exec)
		void SpawnEnemies();

	UFUNCTION(Exec)
		void ChangeSeasonAffect(FString Season);

	UFUNCTION(Exec)
		void AddEnemies(FString Category, int32 Amount);

	UFUNCTION(Exec)
		void CompleteResearch();

	UFUNCTION(Exec)
		void TurnOnInstantBuild(bool Value);

	UFUNCTION(Exec)
		void SpawnCitizen(int32 Amount, bool bAdult);

	UFUNCTION(Exec)
		void SetEvent(FString Type, FString Period, int32 Day, int32 StartHour, int32 EndHour, bool bRecurring, bool bFireFestival = false);

	UFUNCTION(Exec)
		void DamageActor(int32 Amount);

	UFUNCTION(Exec)
		void SetOnFire();

	UFUNCTION(Exec)
		void GiveProblem(bool bInjury);
};
