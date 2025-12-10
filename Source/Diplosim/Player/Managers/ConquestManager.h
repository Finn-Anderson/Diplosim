#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "ConquestManager.generated.h"

USTRUCT(BlueprintType)
struct FCultureImageStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FString Party;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FString Religion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		UTexture2D* Texture;

	FCultureImageStruct()
	{
		Party = "";
		Religion = "";
		Texture = nullptr;
	}

	bool operator==(const FCultureImageStruct& other) const
	{
		return (other.Party == Party && other.Religion == Religion);
	}
};

USTRUCT(BlueprintType)
struct FGiftStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift")
		TSubclassOf<class AResource> Resource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift")
		int32 Amount;

	FGiftStruct()
	{
		Resource = nullptr;
		Amount = 0;
	}
};

USTRUCT(BlueprintType)
struct FAIBuildStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TSubclassOf<class ABuilding> Building;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		int32 NumCitizens;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		int32 NumCitizensIncrement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build")
		int32 CurrentAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		int32 Limit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TSubclassOf<class AResource> Resource;

	FAIBuildStruct()
	{
		Building = nullptr;
		NumCitizens = 0;
		NumCitizensIncrement = 50;
		CurrentAmount = 0;
		Limit = 0;
		Resource = nullptr;
	}

	bool operator==(const FAIBuildStruct& other) const
	{
		return (other.Building == Building);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UConquestManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UConquestManager();

protected:
	virtual void BeginPlay() override;

public:	
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

	void SetFactionFlagColour(FFactionStruct* Faction);

	UTexture2D* GetTextureFromCulture(FString Party, FString Religion);

	UFUNCTION(BlueprintCallable)
		FPoliticsStruct GetFactionPoliticsStruct(FString FactionName);

	UFUNCTION(BlueprintCallable)
		FFactionStruct GetFactionFromActor(AActor* Actor);

	UFUNCTION(BlueprintCallable)
		bool DoesFactionContainActor(FString FactionName, AActor* Actor);

	void ComputeAI();

	UFUNCTION(BlueprintCallable)
		FFactionStruct GetCitizenFaction(class ACitizen* Citizen);

	FFactionStruct* GetFaction(FString Name = "", AActor* Actor = nullptr);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		int32 AINum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		int32 MaxAINum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culture Texture List")
		TArray<FCultureImageStruct> CultureTextureList;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Factions")
		TArray<FFactionStruct> Factions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Factions")
		TArray<FFactionStruct> FactionsToRemove;

	FCriticalSection ConquestLock;

	// Diplomacy
	void SetFactionCulture(FFactionStruct* Faction);

	UFUNCTION(BlueprintCallable)
		int32 GetHappinessWithFaction(FFactionStruct Faction, FFactionStruct Target);

	UFUNCTION(BlueprintCallable)
		int32 GetHappinessValue(FFactionHappinessStruct Happiness);

	void SetFactionsHappiness(FFactionStruct Faction);

	void EvaluateDiplomacy(FFactionStruct Faction);

	int32 GetStrengthScore(FFactionStruct Faction);

	TTuple<bool, bool> IsWarWinnable(FFactionStruct Faction, FFactionStruct& Target);

	UFUNCTION(BlueprintCallable)
		void Peace(FFactionStruct Faction1, FFactionStruct Faction2);

	UFUNCTION(BlueprintCallable)
		void Ally(FFactionStruct Faction1, FFactionStruct Faction2);

	UFUNCTION(BlueprintCallable)
		void BreakAlliance(FFactionStruct Faction1, FFactionStruct Faction2);

	UFUNCTION(BlueprintCallable)
		void DeclareWar(FFactionStruct Faction1, FFactionStruct Faction2);

	UFUNCTION(BlueprintCallable)
		void Insult(FFactionStruct Faction, FFactionStruct Target);

	UFUNCTION(BlueprintCallable)
		void Praise(FFactionStruct Faction, FFactionStruct Target);

	UFUNCTION(BlueprintCallable)
		void Gift(FFactionStruct Faction, TArray<FGiftStruct> Gifts);

	// Tile Distance Caluclation
	double CalculateTileDistance(FVector EggTimerLocation, FVector TileLocation);

	void InitialiseTileLocationDistances(FFactionStruct* Faction);

	void RecalculateTileLocationDistances(FFactionStruct* Faction);

	void RemoveTileLocations(FFactionStruct* Faction, class ABuilding* Building);

	void SortTileDistances(TMap<FVector, double>& Locations);

	TArray<FVector> ConnectRoadTiles(FFactionStruct* Faction, struct FTileStruct* Tile, FVector Location);

	// AI Building
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TArray<FAIBuildStruct> AIBuilds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TArray<TSubclassOf<class ABuilding>> Houses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TArray<TSubclassOf<class ABuilding>> MiscBuilds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TSubclassOf<class ABuilding> RoadClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TSubclassOf<class ABuilding> RampClass;

	void EvaluateAIBuild(FFactionStruct* Faction);

	void AITrade(FFactionStruct* Faction);

	void BuildFirstBuilder(FFactionStruct* Faction);

	void BuildAIBuild(FFactionStruct* Faction);

	void BuildAIHouse(FFactionStruct* Faction);

	void BuildAIRoads(FFactionStruct* Faction);

	void BuildMiscBuild(FFactionStruct* Faction);

	void BuildAIAccessibility(FFactionStruct* Faction);

	void ChooseBuilding(FFactionStruct* Faction, TArray<TSubclassOf<class ABuilding>> BuildingsClasses);

	bool AIValidBuildingLocation(FFactionStruct* Faction, class ABuilding* Building, float Extent, FVector Location);

	bool AICanAfford(FFactionStruct* Faction, TSubclassOf<class ABuilding> BuildingClass, int32 Amount = 1);

	void AIBuild(FFactionStruct* Faction, TSubclassOf<class ABuilding> BuildingClass, TSubclassOf<class AResource> Resource, bool bAccessibility = false, FTileStruct* Tile = nullptr);

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
};
