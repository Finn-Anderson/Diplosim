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

	void PoliceInteraction(FFactionStruct* Faction, class ACitizen* Citizen, float Reach);

	void CalculateIfFight(FFactionStruct* Faction, class ACitizen* Citizen1, class ACitizen* Citizen2, float Citizen1Aggressiveness, float Citizen2Aggressiveness);

	void RespondToReports(FFactionStruct* Faction);

	bool IsCarelessWitness(class ACitizen* Citizen);

	void CreatePoliceReport(FFactionStruct* Faction, class ACitizen* Witness, class ACitizen* Accused, EReportType ReportType, int32 Index);

	bool IsInAPoliceReport(class ACitizen* Citizen, FFactionStruct* Faction);

	void ChangeReportToMurder(class ACitizen* Citizen);

	void StopFighting(class ACitizen* Citizen);

	void CeaseAllInternalFighting(FFactionStruct* Faction);

	UFUNCTION()
		void InterrogateWitnesses(FFactionStruct Faction, class ACitizen* Officer, class ACitizen* Citizen);

	UFUNCTION()
		void SetInNearestJail(FFactionStruct Faction, class ACitizen* Officer, class ACitizen* Citizen);

	void ItterateThroughSentences();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Police")
		TSubclassOf<class AWork> PoliceStationClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Police")
		class UNiagaraSystem* ArrestSystem;

	UPROPERTY()
		class ACamera* Camera;

private:
	void GetCloserToFight(class ACitizen* Citizen, class ACitizen* Target, FVector MidPoint);

	void GotoClosestWantedMan(class ACitizen* Officer);

	void Arrest(class ACitizen* Officer, class ACitizen* Citizen);

	FCriticalSection VandalismInteractionsLock;
	FCriticalSection ReportInteractionsLock;
};
