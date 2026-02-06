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

	UFUNCTION(BlueprintCallable)
		TArray<class ABuilding*> GetFactionBuildingsFromClass(TSubclassOf<class ABuilding> BuildingClass);

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

	UFUNCTION(BlueprintCallable)
		float GetBuildingClassAmount(FString FactionName, TSubclassOf<class ABuilding> BuildingClass);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factions")
		TArray<TSubclassOf<class ABuilding>> BuildingClassDefaultAmount;

	// UI
	UFUNCTION(BlueprintCallable)
		void DisplayConquestNotification(FString Message, FString Owner, bool bChoice);

private:
	FCriticalSection ConquestLock;

	FCriticalSection AIFightLock;
	FCriticalSection BuildingFightLock;
};
