#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BioComponent.h"
#include "Buildings/Work/Work.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/ConstructionManager.h"
#include "Universal/Resource.h"
#include "Universal/DiplosimGameModeBase.h"
#include "DiplosimSaveGame.generated.h"

USTRUCT()
struct FHISMData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY();
		TArray<FTransform> Transforms;

	UPROPERTY();
		TArray<float> CustomDataValues;

	FHISMData()
	{

	}
};

USTRUCT()
struct FCapacityData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString CitizenName;

	UPROPERTY()
		TArray<FString> VisitorNames;

	UPROPERTY()
		float Amount;

	UPROPERTY()
		TMap<int32, EWorkType> WorkHours;

	UPROPERTY()
		bool bBlocked;

	FCapacityData()
	{
		CitizenName = "";
		Amount = 0.0f;
		bBlocked = false;
	}
};

USTRUCT()
struct FBuildingData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString FactionName;

	UPROPERTY()
		int32 Capacity;

	UPROPERTY()
		int32 Seed;

	UPROPERTY()
		FLinearColor ChosenColour;

	UPROPERTY()
		int32 Tier;

	UPROPERTY()
		TArray<FItemStruct> TargetList;

	UPROPERTY()
		TArray<FSocketStruct> SocketList;

	UPROPERTY()
		TArray<FItemStruct> Storage;

	UPROPERTY()
		TArray<FBasketStruct> Basket;

	UPROPERTY()
		bool bOperate;

	UPROPERTY()
		double DeathTime;

	UPROPERTY()
		TArray<FCapacityData> OccupiedData;

	FBuildingData()
	{
		FactionName = "";
		Capacity = 0;
		Seed = 0;
		ChosenColour = FLinearColor();
		Tier = 1;
		DeathTime = 0.0f;
		bOperate = true;
	}
};

USTRUCT()
struct FCitizenData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FVector EnterLocation;

	UPROPERTY()
		bool bRebel;

	// Carrying
	UPROPERTY()
		UClass* ResourceCarryClass;

	UPROPERTY()
		int32 CarryAmount;

	// Bio
	UPROPERTY()
		FString MothersName;

	UPROPERTY()
		FString FathersName;

	UPROPERTY()
		FString PartnersName;

	UPROPERTY()
		int32 HoursTogetherWithPartner;

	UPROPERTY()
		bool bMarried;

	UPROPERTY()
		ESex Sex;

	UPROPERTY()
		ESexuality Sexuality;

	UPROPERTY()
		int32 Age;

	UPROPERTY()
		FString Name;

	UPROPERTY()
		int32 EducationLevel;

	UPROPERTY()
		int32 EducationProgress;

	UPROPERTY()
		int32 PaidForEducationLevel;

	UPROPERTY()
		TArray<FString> ChildrensNames;

	UPROPERTY()
		TArray<FString> SiblingsNames;

	UPROPERTY()
		bool bAdopted;

	// Misc
	UPROPERTY()
		FSpiritualStruct Spirituality;

	UPROPERTY()
		TArray<float> TimeOfAcquirement;

	UPROPERTY()
		float VoicePitch;

	UPROPERTY()
		int32 Balance;

	UPROPERTY()
		TMap<int32, float> HoursWorked;

	UPROPERTY()
		int32 Hunger;

	UPROPERTY()
		int32 Energy;

	UPROPERTY()
		bool bGain;

	UPROPERTY()
		float SpeedBeforeOld;

	UPROPERTY()
		float MaxHealthBeforeOld;

	UPROPERTY()
		bool bHasBeenLeader;

	UPROPERTY()
		EAttendStatus MassStatus; 
		
	UPROPERTY()
		TArray<FConditionStruct> HealthIssues;

	UPROPERTY()
		TMap<FString, int32> Modifiers;

	UPROPERTY()
		int32 SadTimer;

	UPROPERTY()
		EAttendStatus FestivalStatus;

	UPROPERTY()
		bool bConversing;

	UPROPERTY()
		TMap<EHappinessType, int32> DecayingHappiness;

	UPROPERTY()
		TArray<FGeneticsStruct> Genetics;

	UPROPERTY()
		bool bSleep;

	UPROPERTY()
		TArray<int32> HoursSleptToday;

	UPROPERTY()
		TArray<FString> PersonalityTraits;

	FCitizenData()
	{
		EnterLocation = FVector::Zero();
		bRebel = false;

		ResourceCarryClass = nullptr;
		CarryAmount = 0;

		MothersName = "";
		FathersName = "";
		PartnersName = "";
		HoursTogetherWithPartner = 0;
		bMarried = false;
		Sex = ESex::NaN;
		Sexuality = ESexuality::NaN;
		Age = 0;
		EducationLevel = 0;
		EducationProgress = 0;
		PaidForEducationLevel = 0;
		Name = "Citizen";
		bAdopted = false;

		VoicePitch = 1.0f;
		Balance = 0;
		Hunger = 100;
		Energy = 100;
		bGain = false;
		SpeedBeforeOld = 200.0f;
		MaxHealthBeforeOld = 100.0f;
		bHasBeenLeader = false;
		MassStatus = EAttendStatus::Neutral;
		SadTimer = 0;
		FestivalStatus = EAttendStatus::Neutral;
		bConversing = false;
		bSleep = false;

		DecayingHappiness.Add(EHappinessType::Conversation, 0);
		DecayingHappiness.Add(EHappinessType::FamilyDeath, 0);
		DecayingHappiness.Add(EHappinessType::WitnessedDeath, 0);
		DecayingHappiness.Add(EHappinessType::Divorce, 0);
	}
};

USTRUCT()
struct FAIMovementData
{
	GENERATED_USTRUCT_BODY()

	// AI Movement
	UPROPERTY()
		TArray<FVector> Points;

	UPROPERTY()
		FAnimStruct CurrentAnim;

	UPROPERTY()
		double LastUpdatedTime;

	UPROPERTY()
		FTransform Transform;

	UPROPERTY()
		FString ActorToLookAtName;

	// AI Controller
	UPROPERTY()
		FString ChosenBuildingName;

	UPROPERTY()
		FString ActorName;

	UPROPERTY()
		FString LinkedPortalName;

	UPROPERTY()
		FString UltimateGoalName;

	UPROPERTY()
		int32 Instance;

	UPROPERTY()
		FVector Location;

	FAIMovementData()
	{
		LastUpdatedTime = 0.0f;
		Transform = FTransform();
		ActorToLookAtName = "";

		ChosenBuildingName = "";
		ActorName = "";
		LinkedPortalName = "";
		UltimateGoalName = "";
		Instance = 0;
		Location = FVector::Zero();
	}
};

USTRUCT()
struct FAIData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString FactionName;

	UPROPERTY()
		FString BuildingAtName;

	UPROPERTY()
		FAIMovementData MovementData;

	UPROPERTY()
		FLinearColor Colour;

	UPROPERTY()
		FCitizenData CitizenData;

	UPROPERTY()
		bool bSnake;

	FAIData()
	{
		FactionName = "";
		BuildingAtName = "";
		Colour = FLinearColor();
		bSnake = false;
	}
};

USTRUCT()
struct FConstructionData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString BuildingName;

	UPROPERTY()
		FString BuilderName;

	UPROPERTY()
		EBuildStatus Status;

	UPROPERTY()
		int32 BuildPercentage;

	FConstructionData()
	{
		BuildingName = "";
		BuilderName = "";
		Status = EBuildStatus::Construction;
		BuildPercentage = 0;
	}
};

USTRUCT()
struct FWaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FVector> SpawnLocations;

	UPROPERTY()
		TMap<FString, int32> DiedTo;

	UPROPERTY()
		TArray<FString> Threats;

	UPROPERTY()
		int32 NumKilled;

	UPROPERTY()
		TArray<FEnemiesStruct> EnemiesData;

	FWaveData()
	{
		SpawnLocations = {};
		DiedTo = {};
		Threats = {};
		NumKilled = 0;
		EnemiesData = {};
	}
};

USTRUCT()
struct FGamemodeData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FWaveData> WaveData;

	UPROPERTY()
		TArray<FString> EnemyNames;

	UPROPERTY()
		TArray<FString> SnakeNames;

	UPROPERTY()
		bool bOngoingRaid;

	UPROPERTY()
		float CrystalOpacity;

	UPROPERTY()
		float TargetOpacity;
	
	FGamemodeData()
	{
		bOngoingRaid = false;
		CrystalOpacity = 0.0f;
		TargetOpacity = 0.0f;
	}
};

USTRUCT(BlueprintType)
struct FPersonalityData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString Trait;

	UPROPERTY()
		TArray<FString> CitizenNames;

	FPersonalityData()
	{
		Trait = "";
	}
};

USTRUCT()
struct FCitizenManagerData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FPersonalityData> PersonalitiesData;

	UPROPERTY()
		TArray<FString> InfectibleNames;

	UPROPERTY()
		TArray<FString> InfectedNames;

	UPROPERTY()
		TArray<FString> InjuredNames;

	UPROPERTY()
		int32 IssuePensionHour;

	FCitizenManagerData()
	{
		IssuePensionHour = 18;
	}
};

USTRUCT()
struct FPartyData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString Party;

	UPROPERTY()
		TArray<FString> Agreeable;

	UPROPERTY()
		TMap<FString, TEnumAsByte<ESway>> MembersName;

	UPROPERTY()
		FString LeaderName;

	FPartyData()
	{
		Party = "";
		LeaderName = "";
	}
};

USTRUCT()
struct FVoteData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FString> ForNames;

	UPROPERTY()
		TArray<FString> AgainstNames;

	FVoteData()
	{

	}
};

USTRUCT()
struct FPoliticsData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FPartyData> PartiesData;

	UPROPERTY()
		TArray<FString> RepresentativesNames;

	UPROPERTY()
		TArray<int32> BribeValue;

	UPROPERTY()
		TArray<FLawStruct> Laws;

	UPROPERTY()
		FVoteData VotesData;

	UPROPERTY()
		FVoteData PredictionsData;

	UPROPERTY()
		TArray<FLawStruct> ProposedBills;

	FPoliticsData()
	{

	}
};

USTRUCT()
struct FFightTeamData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString InstigatorName;

	UPROPERTY()
		TArray<FString> AssistorsNames;

	FFightTeamData()
	{
		InstigatorName = "";
	}
};

USTRUCT()
struct FPoliceReportData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		EReportType Type;

	UPROPERTY()
		FVector Location;

	UPROPERTY()
		FFightTeamData Team1;

	UPROPERTY()
		FFightTeamData Team2;

	UPROPERTY()
		TMap<FString, float> WitnessesNames;

	UPROPERTY()
		FString RespondingOfficerName;

	UPROPERTY()
		TArray<FString> AcussesTeam1Names;

	UPROPERTY()
		TArray<FString> ImpartialNames;

	UPROPERTY()
		TArray<FString> AcussesTeam2Names;

	FPoliceReportData()
	{
		Type = EReportType::Fighting;
		Location = FVector::Zero();
		RespondingOfficerName = "";
	}
};

USTRUCT(BlueprintType)
struct FPoliceData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FPoliceReportData> PoliceReportsData;

	UPROPERTY()
		TMap<FString, int32> ArrestedNames;

	FPoliceData()
	{

	}
};

USTRUCT()
struct FEventData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		EEventType Type;

	UPROPERTY()
		FString Period;

	UPROPERTY()
		int32 Day;

	UPROPERTY()
		TArray<int32> Hours;

	UPROPERTY()
		bool bRecurring;

	UPROPERTY()
		bool bFireFestival;

	UPROPERTY()
		bool bStarted;

	UPROPERTY()
		TSubclassOf<class ABuilding> Building;

	UPROPERTY()
		FString VenueName;

	UPROPERTY()
		TArray<FString> WhitelistNames;

	UPROPERTY()
		TArray<FString> AttendeesNames;

	UPROPERTY()
		FVector Location;

	FEventData()
	{
		Type = EEventType::Holliday;
		Period = "";
		Day = 0;
		bRecurring = false;
		bStarted = false;
		bFireFestival = false;
		Building = nullptr;
		VenueName = "";
		Location = FVector::Zero();
	}
};

USTRUCT()
struct FArmyData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FString> CitizensNames;

	UPROPERTY()
		bool bGroup;

	FArmyData()
	{
		bGroup = false;
	}
};

USTRUCT()
struct FFactionData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString Name;

	UPROPERTY()
		FLinearColor FlagColour;

	UPROPERTY()
		TArray<FString> AtWar;

	UPROPERTY()
		TArray<FString> Allies;

	UPROPERTY()
		TArray<FFactionHappinessStruct> Happiness;

	UPROPERTY()
		int32 WarFatigue;

	UPROPERTY()
		FString PartyInPower;

	UPROPERTY()
		FString LargestReligion;

	UPROPERTY()
		int32 RebelCooldownTimer;

	UPROPERTY()
		TArray<FResearchStruct> ResearchStruct;

	UPROPERTY()
		TArray<int32> ResearchIndices;

	UPROPERTY()
		FPoliticsData PoliticsData;

	UPROPERTY()
		FPoliceData PoliceData;

	UPROPERTY()
		TArray<FEventData> EventsData;

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

	UPROPERTY()
		int32 FailedBuild;

	UPROPERTY()
		TArray<FArmyData> ArmiesData;

	UPROPERTY()
		TArray<float> BuildingClassAmount;

	FFactionData()
	{
		Name = "";
		FlagColour = FLinearColor();
		WarFatigue = 0;
		PartyInPower = "";
		LargestReligion = "";
		RebelCooldownTimer = 0;
		FailedBuild = 0;
	}
};

USTRUCT()
struct FCameraData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString ColonyName;

	UPROPERTY()
		TArray<FConstructionData> ConstructionData;

	UPROPERTY()
		FGamemodeData GamemodeData;

	UPROPERTY()
		FCitizenManagerData CitizenManagerData;

	UPROPERTY()
		TArray<FFactionData> FactionData;

	FCameraData()
	{
		ColonyName = "";
	}
};

USTRUCT()
struct FActorSaveData
{
	GENERATED_USTRUCT_BODY()

	AActor* Actor;

	UPROPERTY()
		FString Name;

	UPROPERTY()
		TMap<FString, UClass*> Classes;

	UPROPERTY()
		TMap<FString, FTransform> Transforms;

	UPROPERTY()
		TMap<FString, double> Numbers;

	UPROPERTY()
		TMap<FString, FString> Strings;

	UPROPERTY()
		TMap<FString, bool> Bools;

	UPROPERTY()
		TMap<FString, FHISMData> HISMData;
	// Make ambiguous (TMaps with variable name as string key)

	UPROPERTY()
		FBuildingData BuildingData;

	UPROPERTY()
		FAIData AIData;

	UPROPERTY()
		FCameraData CameraData;

	UPROPERTY()
		TArray<FTimerStruct> SavedTimers;

	FActorSaveData()
	{

	}

	bool operator==(const FActorSaveData& other) const
	{
		return (other.Name == Name);
	}
};

USTRUCT(BlueprintType)
struct FSave
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		FString SaveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		FString Period;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		int32 Day;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		int32 Hour;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		int32 CitizenNum;

	UPROPERTY()
		FRandomStream Stream;

	UPROPERTY()
		bool bAutosave;

	UPROPERTY()
		TArray<FActorSaveData> SavedActors;

		FSave()
		{
			SaveName = "";
			Period = "";
			Day = 0;
			Hour = 0;
			CitizenNum = 0;
			bAutosave = false;
		}

		bool operator==(const FSave& other) const
		{
			return (other.SaveName == SaveName);
		}
};

UCLASS()
class DIPLOSIM_API UDiplosimSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	TArray<FSave> Saves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FString ColonyName;

	UPROPERTY()
	FDateTime LastTimeUpdated;

	void SaveGame(class ACamera* Camera, int32 Index, FString ID);

	void LoadGame(class ACamera* Camera, int32 Index);

private:
	template<typename T>
	void SetActorData(FString ID, T Value, FActorSaveData& ActorSaveData)
	{
		if constexpr (std::is_same_v<T, FVector> || std::is_same_v<T, FQuat> || std::is_same_v<T, FRotator>) {
			FTransform transform = FTransform(FQuat::Identity, FVector(0.0f));

			if constexpr (std::is_same_v<T, FVector>)
				transform.SetLocation(Value);
			else if constexpr (std::is_same_v<T, FQuat>)
				transform.SetRotation(Value);
			else
				transform.SetRotation(FQuat(Value));

			ActorSaveData.Transforms.Add(ID, transform);
		}
		else if constexpr (std::is_same_v<T, FTransform>)
			ActorSaveData.Transforms.Add(ID, Value);
		else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float> || std::is_same_v<T, int32>)
			ActorSaveData.Numbers.Add(ID, Value);
		else if constexpr (std::is_same_v<T, FString>)
			ActorSaveData.Strings.Add(ID, Value);
		else if constexpr (std::is_same_v<T, bool>)
			ActorSaveData.Bools.Add(ID, Value);
		else if constexpr (std::is_same_v<T, FHISMData>)
			ActorSaveData.HISMData.Add(ID, Value);
		else if constexpr (std::is_base_of_v<UClass, std::remove_pointer_t<T>>)
			ActorSaveData.Classes.Add(ID, Value);
		else
			static_assert(false, "Not a valid type");
	}

	template<typename T>
	T GetActorData(FString ID, FActorSaveData& ActorSaveData)
	{
		if constexpr (std::is_same_v<T, FVector> || std::is_same_v<T, FQuat> || std::is_same_v<T, FRotator>) {
			FTransform transform = *ActorSaveData.Transforms.Find(ID);

			if constexpr (std::is_same_v<T, FVector>)
				return transform.GetLocation();
			else if constexpr (std::is_same_v<T, FQuat>)
				return transform.GetRotation();
			else
				return transform.GetRotation().Rotator();
		}
		else if constexpr (std::is_same_v<T, FTransform>) {
			FTransform transform = *ActorSaveData.Transforms.Find(ID);
			return transform;
		}
		else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float> || std::is_same_v<T, int32>) {
			double number = *ActorSaveData.Numbers.Find(ID);
			return number;
		}
		else if constexpr (std::is_same_v<T, FString>) {
			FString string = *ActorSaveData.Strings.Find(ID);
			return string;
		}
		else if constexpr (std::is_same_v<T, bool>) {
			bool bBool = *ActorSaveData.Bools.Find(ID);
			return bBool;
		}
		else if constexpr (std::is_same_v<T, FHISMData>) {
			FHISMData data = *ActorSaveData.HISMData.Find(ID);
			return data;
		}
		else if constexpr (std::is_base_of_v<UClass, std::remove_pointer_t<T>>) {
			UClass* foundClass = *ActorSaveData.Classes.Find(ID);
			return foundClass;
		}
		else
			static_assert(false, "Not a valid type");
	}

	// Saving
	void SaveWorld(FActorSaveData& ActorData, AActor* Actor, int32 Index, TArray<AActor*> PotentialWetActors);

	void SaveResource(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor);

	void SaveCamera(FActorSaveData& ActorData, AActor* Actor);
	void SaveCitizenManager(FActorSaveData& ActorData, AActor* Actor);
	void SaveFactions(FActorSaveData& ActorData, AActor* Actor);
	void SaveGamemode(FActorSaveData& ActorData, AActor* Actor);

	void SaveAI(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor);
	void SaveCitizen(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor);

	void SaveBuilding(FActorSaveData& ActorData, AActor* Actor);

	void SaveProjectile(FActorSaveData& ActorData, AActor* Actor);

	void SaveAISpawner(FActorSaveData& ActorData, AActor* Actor);

	void SaveTimers(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor);

	void SaveComponents(FActorSaveData& ActorData, AActor* Actor);

	// Loading
	void LoadWorld(FActorSaveData& ActorData, AActor* Actor, int32 Index, TMap<FString, FActorSaveData>& WetNames);

	void LoadResource(FActorSaveData& ActorData, AActor* Actor);

	void LoadEggBasket(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor);

	void LoadCamera(FActorSaveData& ActorData, AActor* Actor);
	void LoadFactions(FActorSaveData& ActorData, AActor* Actor);
	void LoadGamemode(class ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, AActor* Actor);

	void LoadAI(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, TMap<FString, FActorSaveData*>& AIToName);
	void LoadCitizen(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor);

	void LoadBuilding(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, TMap<FString, FActorSaveData*>& AIToName);

	void LoadProjectile(FActorSaveData& ActorData, AActor* Actor);

	void LoadAISpawner(FActorSaveData& ActorData, AActor* Actor);

	void LoadComponents(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, TArray<FActorSaveData> SavedData);

	void LoadTimers(class ACamera* Camera, int32 Index, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData);

	void InitialiseObjects(class ACamera* Camera, class ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData);

	void InitialiseAI(class ACamera* Camera, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData);
	void InitialiseCitizen(class ACamera* Camera, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData);

	void InitialiseConstructionManager(class ACamera* Camera, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData);
	void InitialiseCitizenManager(class ACamera* Camera, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData);
	void InitialiseFactions(class ACamera* Camera, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData);
	void InitialiseGamemode(class ACamera* Camera, class ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData);

	void InitialiseResources(class ACamera* Camera, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData);
};
