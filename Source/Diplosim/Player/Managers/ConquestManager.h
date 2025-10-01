#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Universal/Resource.h"
#include "Buildings/Building.h"
#include "Components/ActorComponent.h"
#include "ConquestManager.generated.h"

//
// Politics
//
USTRUCT(BlueprintType)
struct FVoteStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<class ACitizen*> For;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<class ACitizen*> Against;

	FVoteStruct()
	{

	}

	void Clear()
	{
		For.Empty();
		Against.Empty();
	}
};

USTRUCT(BlueprintType)
struct FPoliticsStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FPartyStruct> Parties;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Politics")
		TArray<class ACitizen*> Representatives;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Politics")
		TArray<int32> BribeValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FLawStruct> Laws;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Politics")
		FVoteStruct Votes;

	UPROPERTY()
		FVoteStruct Predictions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FLawStruct> ProposedBills;

	FPoliticsStruct()
	{
		
	}
};

//
// Police
//
UENUM()
enum class EReportType : uint8
{
	Fighting,
	Murder,
	Vandalism,
	Protest
};

USTRUCT()
struct FFightTeam
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		class ACitizen* Instigator;

	UPROPERTY()
		TArray<class ACitizen*> Assistors;

	FFightTeam()
	{
		Instigator = nullptr;
	}

	TArray<class ACitizen*> GetTeam()
	{
		TArray<class ACitizen*> team = Assistors;
		team.Add(Instigator);

		return team;
	}

	bool HasCitizen(class ACitizen* Citizen)
	{
		if (Instigator == Citizen || Assistors.Contains(Citizen))
			return true;

		return false;
	}

	bool operator==(const FFightTeam& other) const
	{
		return (other.Instigator == Instigator);
	}
};

USTRUCT()
struct FPoliceReport
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		EReportType Type;

	UPROPERTY()
		FVector Location;

	UPROPERTY()
		FFightTeam Team1;

	UPROPERTY()
		FFightTeam Team2;

	UPROPERTY()
		TMap<class ACitizen*, float> Witnesses;

	UPROPERTY()
		class ACitizen* RespondingOfficer;

	UPROPERTY()
		TArray<class ACitizen*> AcussesTeam1;

	UPROPERTY()
		TArray<class ACitizen*> Impartial;

	UPROPERTY()
		TArray<class ACitizen*> AcussesTeam2;

	FPoliceReport()
	{
		Type = EReportType::Fighting;
		Location = FVector::Zero();
		RespondingOfficer = nullptr;
	}

	bool Contains(class ACitizen* Citizen)
	{
		if (Team1.Instigator == Citizen || Team1.Assistors.Contains(Citizen) || Team2.Instigator == Citizen || Team2.Assistors.Contains(Citizen))
			return true;

		return false;
	}

	bool operator==(const FPoliceReport& other) const
	{
		return (other.Team1 == Team1 && other.Team2 == Team2);
	}
};

USTRUCT(BlueprintType)
struct FPoliceStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FPoliceReport> PoliceReports;

	UPROPERTY()
		TMap<ACitizen*, int32> Arrested;

	FPoliceStruct()
	{

	}
};

//
// Resources
//
USTRUCT(BlueprintType)
struct FFactionResourceStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Committed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 LastHourAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TMap<int32, int32> HourlyTrend;

	FFactionResourceStruct()
	{
		Type = nullptr;
		Committed = 0;
		LastHourAmount = 0;
		for (int32 i = 0; i < 24; i++)
			HourlyTrend.Add(i, 0);
	}

	bool operator==(const FFactionResourceStruct& other) const
	{
		return (other.Type == Type);
	}
};

//
// Factions
//
USTRUCT(BlueprintType)
struct FFactionHappinessStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Happiness")
		FString Owner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Happiness")
		TMap<FString, int32> Modifiers;

	int32 ProposalTimer;

	FFactionHappinessStruct()
	{
		ClearValues();
		ProposalTimer = 0;
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
struct FArmyStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Army")
		TArray<class ACitizen*> Citizens;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Army")
		class UWidgetComponent* WidgetComponent;

	UPROPERTY()
		bool bGroup;

	FArmyStruct()
	{
		WidgetComponent = nullptr;
		bGroup = false;
	}
};

USTRUCT(BlueprintType)
struct FFactionStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		UTexture2D* Flag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FLinearColor FlagColour;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		TArray<FString> AtWar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		TArray<FString> Allies;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		TArray<FFactionHappinessStruct> Happiness;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		int32 WarFatigue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FString PartyInPower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FString LargestReligion;

	UPROPERTY()
		TArray<class ACitizen*> Citizens;

	UPROPERTY()
		TArray<class AAI*> Clones;

	UPROPERTY()
		TArray<class ACitizen*> Rebels;

	UPROPERTY()
		int32 RebelCooldownTimer;

	UPROPERTY()
		TArray<ABuilding*> Buildings;

	UPROPERTY()
		TArray<ABuilding*> RuinedBuildings;

	UPROPERTY()
		class ABroch* EggTimer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		TArray<FResearchStruct> ResearchStruct;

	UPROPERTY()
		int32 ResearchIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FPoliticsStruct Politics;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		FPoliceStruct Police;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		TArray<FEventStruct> Events;

	UPROPERTY()
		FPrayStruct PrayStruct;

	UPROPERTY()
		TArray<FFactionResourceStruct> Resources;

	UPROPERTY()
		TMap<FVector, double> AccessibleBuildLocations;

	UPROPERTY()
		TArray<FVector> InaccessibleBuildLocations;

	UPROPERTY()
		TArray<FVector> RoadBuildLocations;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Army")
		TArray<FArmyStruct> Armies;

	FFactionStruct()
	{
		Name = "";
		WarFatigue = 0;
		PartyInPower = "";
		LargestReligion = "";
		EggTimer = nullptr;
		ResearchIndex = 0;
		RebelCooldownTimer = 0;
	}

	bool operator==(const FFactionStruct& other) const
	{
		return (other.Name == Name);
	}
};

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
		TSubclassOf<ABuilding> Building;

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
		ABuilding* DoesFactionContainUniqueBuilding(FString FactionName, TSubclassOf<ABuilding> BuildingClass);

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
		TArray<FResearchStruct> GetFactionResearch(FString FactionName);

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

	void RemoveTileLocations(FFactionStruct* Faction, ABuilding* Building);

	void SortTileDistances(FFactionStruct* Faction);

	TArray<FVector> ConnectRoadTiles(FFactionStruct* Faction, struct FTileStruct* Tile, FVector Location);

	// AI Building
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TArray<FAIBuildStruct> AIBuilds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TArray<TSubclassOf<ABuilding>> Houses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TArray<TSubclassOf<ABuilding>> MiscBuilds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TSubclassOf<ABuilding> RoadClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TSubclassOf<ABuilding> RampClass;

	UPROPERTY()
		int32 FailedBuild;

	void EvaluateAIBuild(FFactionStruct* Faction);

	void AITrade(FFactionStruct* Faction);

	void BuildFirstBuilder(FFactionStruct* Faction);

	void BuildAIBuild(FFactionStruct* Faction);

	void BuildAIHouse(FFactionStruct* Faction);

	void BuildAIRoads(FFactionStruct* Faction);

	void BuildMiscBuild(FFactionStruct* Faction);

	void BuildAIAccessibility(FFactionStruct* Faction);

	void ChooseBuilding(FFactionStruct* Faction, TArray<TSubclassOf<ABuilding>> BuildingsClasses);

	bool AIValidBuildingLocation(FFactionStruct* Faction, ABuilding* Building, float Extent, FVector Location);

	bool AICanAfford(FFactionStruct* Faction, TSubclassOf<ABuilding> BuildingClass, int32 Amount = 1);

	void AIBuild(FFactionStruct* Faction, TSubclassOf<ABuilding> BuildingClass, TSubclassOf<class AResource> Resource, bool bAccessibility = false, FTileStruct* Tile = nullptr);

	// AI Army
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> ArmyUI;

	UFUNCTION(BlueprintCallable)
		bool CanJoinArmy(class ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		void CreateArmy(FString FactionName, TArray<class ACitizen*> Citizens);

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

	TArray<ABuilding*> GetReachableTargets(FFactionStruct* Faction, class ACitizen* Citizen);

	void MoveToTarget(FFactionStruct* Faction, class TArray<ACitizen*> Citizens);

	void StartRaid(FFactionStruct* Faction, int32 Index);

	void EvaluateAIArmy(FFactionStruct* Faction);

	class ABuilding* MoveArmyMember(FFactionStruct* Faction, class AAI* AI, bool bReturn = false);

	// UI
	UFUNCTION(BlueprintCallable)
		void DisplayConquestNotification(FString Message, FString Owner, bool bChoice);
};
