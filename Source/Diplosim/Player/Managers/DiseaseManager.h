#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "DiseaseManager.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UDiseaseManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UDiseaseManager();

	void CalculateDisease(class ACamera* Camera);

	void StartDiseaseTimer(class ACamera* Camera);

	UFUNCTION()
		void SpawnDisease(class ACamera* Camera);

	void Infect(class ACitizen* Citizen);

	void Cure(class ACitizen* Citizen);

	void Injure(class ACitizen* Citizen, int32 Odds);

	TArray<ACitizen*> GetAvailableHealers(FFactionStruct* Faction, TArray<ACitizen*>& Ill, ACitizen* Target);

	void PairCitizenToHealer(FFactionStruct* Faction, ACitizen* Healer = nullptr);

	UPROPERTY()
		TArray<class ACitizen*> Infectible;

	UPROPERTY()
		TArray<class ACitizen*> Infected;

	UPROPERTY()
		TArray<class ACitizen*> Injured;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		TArray<FConditionStruct> Diseases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		TArray<FConditionStruct> Injuries;

protected:
	void ReadJSONFile(FString path);

private:
	void UpdateHealthText(class ACitizen* Citizen);

	FCriticalSection DiseaseSpreadLock;
};
