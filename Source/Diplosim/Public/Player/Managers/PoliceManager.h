#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "PoliceManager.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UPoliceManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPoliceManager();

	void CalculateVandalism();

	void ProcessReports();

	void CalculateIfFight(FFactionStruct* Faction, class ACitizen* Citizen1, class ACitizen* Citizen2, float Citizen1Aggressiveness, float Citizen2Aggressiveness);

	bool IsCarelessWitness(class ACitizen* Citizen);

	bool IsPoliceOfficer(class ACitizen* Citizen);

	void CreatePoliceReport(FFactionStruct* Faction, EReportType ReportType, class ACitizen* Accused, bool bShowMercy, AActor* Defender);

	bool IsInAPoliceReport(class ACitizen* Citizen, FFactionStruct* Faction);

	void ChangeReportToMurder(class ACitizen* Citizen, class ACitizen* Attacker);

	void CeaseAllInternalFighting(FFactionStruct* Faction);

	void RepositionCriminals(class ABuilding* Building);

	UFUNCTION()
		void Arrest(FFactionStruct Faction, class ACitizen* Officer, class ACitizen* Citizen);

	UFUNCTION()
		void Jail(FFactionStruct Faction, class ACitizen* Officer, class ACitizen* Citizen);

	void ItterateThroughSentences();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Police")
		TSubclassOf<class AWork> PoliceStationClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Police")
		class UNiagaraSystem* ArrestSystem;

	UPROPERTY()
		class ACamera* Camera;

private:
	FCriticalSection VandalismInteractionsLock;
	FCriticalSection ReportInteractionsLock;
};
