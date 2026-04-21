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

	void GiveCondition(class ACamera* Camera, class ACitizen* Citizen, FConditionStruct Condition);

	void StartDiseaseTimer(class ACamera* Camera);

	UFUNCTION()
		void SpawnDisease(class ACamera* Camera);

	UFUNCTION()
		void Cure(class ACitizen* Healer, class ACitizen* Citizen);

	void Injure(class ACitizen* Citizen, int32 Odds);

	TArray<ACitizen*> GetAvailableHealers(FFactionStruct* Faction, TArray<ACitizen*>& Ill);

	void PairCitizenToHealer(FFactionStruct* Faction);

	TArray<ACitizen*> GetIll(FFactionStruct* Faction, bool bOnlyInfections);

	TTuple<bool, bool> HasInjuryAndInfection(ACitizen* Citizen);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		TArray<FConditionStruct> Diseases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		TArray<FConditionStruct> Injuries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		USoundBase* InjureSound;

protected:
	void ReadJSONFile(FString path);

private:
	void UpdateHealthText(class ACitizen* Citizen);

	bool HasInfection(class ACitizen* Citizen);

	bool IsInfectible(class ACitizen* Citizen);

	FCriticalSection DiseaseSpreadLock;
};
