#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Map/Grid.h"
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

	TTuple<FFactionStruct*, int32> GetArmyIndex(ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		void DestroyArmy(FString FactionName, int32 Index);

	TArray<AActor*> GetReachableTargets(FFactionStruct* Faction, class ACitizen* Citizen);

	void MoveToTarget(FFactionStruct* Faction, TArray<class ACitizen*> Citizens, int32 Index = INDEX_NONE);

	UFUNCTION()
		void StartRaid(FFactionStruct Faction, int32 Index);

	class ABuilding* MoveArmyMember(FFactionStruct* Faction, class AAI* AI, bool bReturn = false, ABuilding* CurrentTarget = nullptr);

	void SetSelectedArmy(TTuple<FFactionStruct*, int32> SelectedArmyData);

	void PlayerMoveArmy(FVector Location);

	TTuple<FFactionStruct*, int32> PlayerSelectedArmyData;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY()
		int32 NumTiles;

	UPROPERTY()
		TMap<FVector, double> ArmyTileLocations;

private:
	void GetArmyTiles(FTileStruct* Tile, int32 Count, FVector Origin);
};
