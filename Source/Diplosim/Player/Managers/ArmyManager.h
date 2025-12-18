#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "ArmyManager.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UArmyManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UArmyManager();

	void EvaluateAIArmy(FFactionStruct* Faction);

	UFUNCTION(BlueprintCallable)
		bool CanJoinArmy(class ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		void CreateArmy(FString FactionName, TArray<class ACitizen*> Citizens, bool bGroup = true, bool bLoad = false);

	UFUNCTION(BlueprintCallable)
		void AddToArmy(int32 Index, TArray<ACitizen*> Citizens);

	UFUNCTION(BlueprintCallable)
		void RemoveFromArmy(ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		void UpdateArmy(FString FactionName, int32 Index, TArray<ACitizen*> AlteredCitizens);

	UFUNCTION(BlueprintCallable)
		bool IsCitizenInAnArmy(ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		void DestroyArmy(FString FactionName, int32 Index);

	TArray<class ABuilding*> GetReachableTargets(FFactionStruct* Faction, class ACitizen* Citizen);

	void MoveToTarget(FFactionStruct* Faction, class TArray<ACitizen*> Citizens);

	UFUNCTION()
		void StartRaid(FFactionStruct Faction, int32 Index);

	class ABuilding* MoveArmyMember(FFactionStruct* Faction, class AAI* AI, bool bReturn = false, ABuilding* CurrentTarget = nullptr);

	UFUNCTION(BlueprintCallable)
		void SetSelectedArmy(int32 Index);

	void PlayerMoveArmy(FVector Location);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> ArmyUI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Army")
		int32 PlayerSelectedArmyIndex;

	UPROPERTY()
		class ACamera* Camera;
};
