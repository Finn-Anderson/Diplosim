#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "ConquestManager.generated.h"

USTRUCT(BlueprintType)
struct FFactionHappinessStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Happiness")
		FString Owner;

	UPROPERTY(BlueprintReadOnly, Category = "Happiness")
		TMap<FString, int32> Modifiers;

	FFactionHappinessStruct()
	{
		ClearValues();
	}

	void ClearValues()
	{
		Modifiers.Empty();
	}
	
	bool Contains(FString Key)
	{
		return Modifiers.Contains(Key);
	}

	int32 GetValue(FString Key)
	{
		return *Modifiers.Find(Key);
	}

	void SetValue(FString Key, int32 Value)
	{
		Modifiers.Add(Key, Value);
	}

	void RemoveValue(FString Key)
	{
		Modifiers.Remove(Key);
	}

	void Decay(FString Key) {
		int32 value = GetValue(Key);

		if (value < 0)
			value++;
		else
			value--;

		if (value != 0)
			SetValue(Key, value);
		else
			RemoveValue(Key);
	}

	bool operator==(const FFactionHappinessStruct& other) const
	{
		return (other.Owner == Owner);
	}
};

USTRUCT(BlueprintType)
struct FFactionStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		UTexture2D* Texture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FLinearColor Colour;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		TArray<FString> AtWar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		TArray<FString> Allies;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		TArray<FFactionHappinessStruct> Happiness;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		int32 WarFatigue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		EParty PartyInPower;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		EReligion Religion;

	FFactionStruct()
	{
		Name = "";
		Texture = nullptr;
		Colour = FLinearColor(1.0f, 1.0f, 1.0f);
		WarFatigue = 0;
		PartyInPower = EParty::Undecided;
		Religion = EReligion::Atheist;
	}

	bool operator==(const FFactionStruct& other) const
	{
		return (other.Name == Name);
	}
};

USTRUCT(BlueprintType)
struct FRaidStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raiding")
		FString Owner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raiding")
		TMap<class ACitizen*, int32> Raiders;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raiding")
		bool bStation;

	FRaidStruct() 
	{
		bStation = false;
	}

	bool operator==(const FRaidStruct& other) const
	{
		return (other.Owner == Owner);
	}
};

USTRUCT(BlueprintType)
struct FStationedStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
	TArray<class ACitizen*> Guards;

	FStationedStruct() {

	}
};

USTRUCT(BlueprintType)
struct FWorldTileStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		int32 X;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		int32 Y;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		bool bIsland;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		bool bCapital;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		FString Owner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		FString Name;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		TSubclassOf<class AResource> Resource;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		int32 Abundance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		TArray<class ACitizen*> Citizens;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		TMap<FString, FStationedStruct> Stationed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		TMap<class ACitizen*, int32> Moving;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		FString RaidStarterName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		TArray<FRaidStruct> RaidParties;

	int32 HoursColonised;

	FWorldTileStruct() {
		X = 0;
		Y = 0;
		bIsland = false;
		bCapital = false;
		Name = "";
		Abundance = 1;
		HoursColonised = 0;
	}

	bool operator==(const FWorldTileStruct& other) const
	{
		return (other.X == X) && (other.Y == Y);
	}
};

USTRUCT(BlueprintType)
struct FIslandResourceStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> ResourceClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Max;

	FIslandResourceStruct()
	{
		ResourceClass = nullptr;
		Min = 0;
		Max = 0;
	}

	bool operator==(const FIslandResourceStruct& other) const
	{
		return (other.ResourceClass == ResourceClass);
	}
};

USTRUCT(BlueprintType)
struct FColonyEventStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		float ResourceMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		float CitizenMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		float Citizens;

	FColonyEventStruct()
	{
		Name = "";
		ResourceMultiplier = 1.0f;
		CitizenMultiplier = 0.0f;
		Citizens = 0.0f;
	}

	bool operator==(const FColonyEventStruct& other) const
	{
		return (other.Name == Name);
	}
};

struct FColoniesStruct
{
	FFactionStruct* Faction;

	FWorldTileStruct* Capital;

	TArray<FWorldTileStruct*> Colonies;

	FColoniesStruct()
	{

	}

	bool operator==(const FColoniesStruct& other) const
	{
		return (other.Faction == Faction);
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
	void GenerateWorld();

	void StartConquest();

	void GiveResource();

	void SpawnCitizenAtColony(FWorldTileStruct& Tile);

	void MoveToColony(FFactionStruct* Faction, FWorldTileStruct* Tile, class ACitizen* Citizen, bool bStation = false);

	void StartTransmissionTimer(class ACitizen* Citizen);

	void AddCitizenToColony(FWorldTileStruct* OldTile, FWorldTileStruct* Tile, class ACitizen* Citizen);

	TArray<float> ProduceEvent();

	FWorldTileStruct* GetColonyContainingCitizen(class ACitizen* Citizen);

	void ModifyCitizensEvent(FWorldTileStruct& Tile, int32 Amount, bool bNegative);

	UFUNCTION(BlueprintCallable)
		bool CanTravel(class ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		FWorldTileStruct GetTileInformation(int32 Index);

	UFUNCTION(BlueprintCallable)
		FFactionStruct& GetFactionFromOwner(FString Owner);

	UFUNCTION(BlueprintCallable)
		void SetFactionTexture(FString Owner, UTexture2D* Texture, FLinearColor Colour);

	UFUNCTION(BlueprintCallable)
		void SetColonyName(int32 X, int32 Y, FString NewColonyName);

	UFUNCTION(BlueprintCallable)
		void SetTerritoryName(FString OldEmpireName, FString NewEmpireName);

		void RemoveFromRecentlyMoved(class ACitizen* Citizen);

	TArray<ACitizen*> GetIslandCitizens(FWorldTileStruct* Tile);

	FWorldTileStruct* FindCapital(FFactionStruct& Faction, TArray<FWorldTileStruct*> OccupiedIslands);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		FString EmpireName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		int32 WorldSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		int32 PercentageIsland;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		int32 EnemiesNum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizens")
		TSubclassOf<class ACitizen> CitizenClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource List")
		TArray<FIslandResourceStruct> ResourceList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event List")
		TArray<FColonyEventStruct> EventList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occupier Texture List")
		TArray<FFactionStruct> DefaultOccupierTextureList;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		TArray<FWorldTileStruct> World;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		TArray<FFactionStruct> Factions;

	UPROPERTY()
		TArray<class ACitizen*> RecentlyMoved;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		int32 playerCapitalIndex;

	UPROPERTY()
		class APortal* Portal;

	// Diplomacy
	void SetFactionCulture(FFactionStruct& Faction);

	UFUNCTION(BlueprintCallable)
		FFactionHappinessStruct& GetHappinessWithFaction(FFactionStruct Faction, FFactionStruct Target);

	int32 GetHappinessValue(FFactionHappinessStruct& Happiness);

	void SetFactionsHappiness(FFactionStruct& Faction, TArray<FWorldTileStruct*> OccupiedIslands);

	void EvaluateDiplomacy(FFactionStruct& Faction);

	TTuple<bool, bool> IsWarWinnable(FFactionStruct& Faction, FFactionStruct& Target);

	void Peace(FFactionStruct& Faction1, FFactionStruct& Faction2);

	void Ally(FFactionStruct* Faction1, FFactionStruct* Faction2);

	void Rebel(FFactionStruct& Faction, TArray<FWorldTileStruct*> OccupiedIslands);

	void Gift(FFactionStruct& Faction, TSubclassOf<class AResource> Resource, int32 Amount);

	UFUNCTION(BlueprintCallable)
		bool CanBuyIsland(FWorldTileStruct Tile);

	int32 GetIslandWorth(FWorldTileStruct Tile);

	void BuyIsland(FWorldTileStruct Tile);

	// Raiding
	UFUNCTION(BlueprintCallable)
		bool IsCurrentlyRaiding(FFactionStruct Faction1, FFactionStruct Faction2);

	UFUNCTION(BlueprintCallable)
		bool CanRaidIsland(FFactionStruct Faction, FWorldTileStruct Tile);

	bool CanStartRaid(FWorldTileStruct* Tile, FFactionStruct* Occupier);

	void EvaluateRaid(FWorldTileStruct* Tile);

	// AI
	void EvaluateAI(FFactionStruct& Faction, TArray<FWorldTileStruct*> OccupiedIslands);

	class ACitizen* GetChosenCitizen(TArray<ACitizen*> Citizens);
};
