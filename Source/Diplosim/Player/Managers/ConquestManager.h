#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "ConquestManager.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UConquestManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UConquestManager();

	FFactionStruct InitialiseFaction(FString Name);

	void CreatePlayerFaction();

	void FinaliseFactions(class ABroch* EggTimer);

	UFUNCTION(BlueprintCallable)
		class ABuilding* DoesFactionContainUniqueBuilding(FString FactionName, TSubclassOf<class ABuilding> BuildingClass);

	int32 GetFactionIndexFromName(FString FactionName);

	UFUNCTION(BlueprintCallable)
		FFactionStruct GetFactionFromName(FString FactionName);

	UFUNCTION(BlueprintCallable)
		TArray<ACitizen*> GetCitizensFromFactionName(FString FactionName);

	UFUNCTION(BlueprintCallable)
		FPoliticsStruct GetFactionPoliticsStruct(FString FactionName);

	UFUNCTION(BlueprintCallable)
		FFactionStruct GetFactionFromActor(AActor* Actor);

	UFUNCTION(BlueprintCallable)
		bool DoesFactionContainActor(FString FactionName, AActor* Actor);

	void ComputeAI();

	void CalculateAIFighting();

	void CalculateBuildingFighting();

	void CheckLoadFactionLock();

	UFUNCTION(BlueprintCallable)
		FFactionStruct GetCitizenFaction(class ACitizen* Citizen);

	FFactionStruct* GetFaction(FString Name = "", AActor* Actor = nullptr);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		class UAIBuildComponent* AIBuildComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		class UDiplomacyComponent* DiplomacyComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		int32 AINum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		int32 MaxAINum;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Factions")
		TArray<FFactionStruct> Factions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Factions")
		TArray<FFactionStruct> FactionsToRemove;

	// AI Army
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> ArmyUI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Army")
		int32 PlayerSelectedArmyIndex;

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

	void EvaluateAIArmy(FFactionStruct* Faction);

	class ABuilding* MoveArmyMember(FFactionStruct* Faction, class AAI* AI, bool bReturn = false);

	UFUNCTION(BlueprintCallable)
		void SetSelectedArmy(int32 Index);

	void PlayerMoveArmy(FVector Location);

	// UI
	UFUNCTION(BlueprintCallable)
		void DisplayConquestNotification(FString Message, FString Owner, bool bChoice);

private:
	FCriticalSection ConquestLock;

	FCriticalSection AIFightLock;
	FCriticalSection BuildingFightLock;
};
