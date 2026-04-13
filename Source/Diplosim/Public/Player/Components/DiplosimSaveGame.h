#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BioComponent.h"
#include "Buildings/Work/Work.h"
#include "Buildings/Work/Service/Trader.h"
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

	UPROPERTY()
		FString Name;

	UPROPERTY();
		TArray<FTransform> Transforms;

	UPROPERTY();
		TArray<float> CustomDataValues;

	FHISMData()
	{
		Name = "";
	}

	bool operator==(const FHISMData& other) const
	{
		return (other.Name == Name);
	}
};
USTRUCT()
struct FAtmosphereData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FCalendarStruct Calendar;

	UPROPERTY()
		FRotator WindRotation;

	UPROPERTY()
		int32 WindSpeed;

	UPROPERTY()
		FRotator SunRotation;

	UPROPERTY()
		FRotator MoonRotation;

	UPROPERTY()
		bool bRedSun;

	FAtmosphereData()
	{
		WindRotation = FRotator::ZeroRotator;
		WindSpeed = 20;
		SunRotation = FRotator(-15.0f, 15.0f, 0.0f);
		MoonRotation = FRotator(15.0f, 195.0f, 0.0f);
		bRedSun = false;
	}
};

USTRUCT()
struct FWetnessData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FVector Location;

	UPROPERTY()
		float Value;

	UPROPERTY()
		float Increment;

	FWetnessData()
	{
		Location = FVector::Zero();
		Value = -1.0f;
		Increment = 0.0f;
	}
};

USTRUCT()
struct FCloudData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FHISMData HISMData;

	UPROPERTY()
		FTransform Transform;
	
	UPROPERTY()
		double Distance;

	UPROPERTY()
		bool bPrecipitation;

	UPROPERTY()
		bool bHide;

	UPROPERTY()
		float lightningTimer;

	UPROPERTY()
		float lightningFrequency;

	UPROPERTY()
		float Opacity;

	FCloudData()
	{
		Transform = FTransform();
		Distance = 0.0f;
		bPrecipitation = false;
		bHide = false;
		lightningTimer = 0.0f;
		lightningFrequency = 0.0f;
		Opacity = 0.0f;
	}
};

USTRUCT()
struct FCloudsData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FWetnessData> WetnessData;

	UPROPERTY()
		TArray<FCloudData> CloudData;

	UPROPERTY()
		bool bSnow;

	FCloudsData()
	{
		bSnow = false;
	}
};

USTRUCT()
struct FNaturalDisasterData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		float DisasterChance;

	UPROPERTY()
		float Intensity;

	UPROPERTY()
		float Frequency;

	FNaturalDisasterData()
	{
		DisasterChance = 0.0f;
		Intensity = 1.0f;
		Frequency = 1.0f;
	}
};

USTRUCT()
struct FTileData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		int32 Level;

	UPROPERTY()
		int32 Fertility;

	UPROPERTY()
		int32 X;

	UPROPERTY()
		int32 Y;

	UPROPERTY()
		FQuat Rotation;

	UPROPERTY()
		bool bRamp;

	UPROPERTY()
		bool bRiver;

	UPROPERTY()
		bool bEdge;

	UPROPERTY()
		bool bMineral;

	UPROPERTY()
		bool bUnique;

	FTileData()
	{
		Level = -1;
		Fertility = -1;
		X = 0;
		Y = 0;
		Rotation = FRotator(0.0f, 0.0f, 0.0f).Quaternion();
		bRamp = false;
		bRiver = false;
		bEdge = false;
		bMineral = false;
		bUnique = false;
	}
};

USTRUCT()
struct FWorkerData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FString> CitizenNames;

	UPROPERTY()
		int32 Instance;

	FWorkerData()
	{
		Instance = INDEX_NONE;
	}
};

USTRUCT()
struct FResourceData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FHISMData HISMData;

	UPROPERTY()
		TArray<int32> FireInstances;

	UPROPERTY()
		TArray<FWorkerData> WorkersData;

	FResourceData()
	{

	}
};

USTRUCT()
struct FWorldSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FHISMData> HISMData;

	UPROPERTY()
		int32 Size;

	UPROPERTY()
		int32 Chunks;

	UPROPERTY()
		FRandomStream Stream;

	UPROPERTY()
		TArray<FVector> LavaSpawnLocations;

	UPROPERTY()
		FAtmosphereData AtmosphereData;

	UPROPERTY()
		FCloudsData	CloudsData;

	UPROPERTY()
		FNaturalDisasterData NaturalDisasterData;

	UPROPERTY()
		double WorldTimer;

	FWorldSaveData()
	{
		Stream = FRandomStream();
		Size = 0;
		Chunks = 0;
		WorldTimer = 0.0f;
	}
};

USTRUCT()
struct FHealthData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int32 Health;

	FHealthData()
	{
		Health = 0;
	}
};

USTRUCT()
struct FAttackData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FString> ActorNames;

	UPROPERTY()
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY()
	float AttackTimer;

	UPROPERTY()
	bool bShowMercy;

	FAttackData()
	{
		ProjectileClass = nullptr;
		AttackTimer = 0.0f;
		bShowMercy = false;
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
		TMap<TSubclassOf<class AResource>, bool> Store;

	UPROPERTY()
		TArray<FQueueStruct> Orders;

	UPROPERTY()
		TArray<FBasketStruct> Basket;

	UPROPERTY()
		bool bOperate;

	UPROPERTY()
		double DeathTime;

	UPROPERTY()
		bool bOnFire;

	UPROPERTY()
		TArray<FCapacityData> OccupiedData;

	UPROPERTY()
		FHealthData HealthData;

	UPROPERTY()
		FAttackData AttackData;

	FBuildingData()
	{
		FactionName = "";
		Capacity = 0;
		Seed = 0;
		ChosenColour = FLinearColor();
		Tier = 1;
		DeathTime = 0.0f;
		bOperate = true;
		bOnFire = false;
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

	UPROPERTY()
		FHealthData HealthData;

	UPROPERTY()
		FAttackData AttackData;


	FAIData()
	{
		FactionName = "";
		BuildingAtName = "";
		Colour = FLinearColor();
		bSnake = false;
	}
};

USTRUCT()
struct FProjectileData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString OwnerName;

	UPROPERTY()
		FVector Velocity;

	FProjectileData()
	{
		OwnerName = "";
		Velocity = FVector::Zero();
	}
};

USTRUCT()
struct FSpawnerData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FLinearColor Colour;

	UPROPERTY()
		int32 IncrementSpawned;

	FSpawnerData()
	{
		Colour = FLinearColor();
		IncrementSpawned = 0;
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

	UPROPERTY()
		FString Target;

	FArmyData()
	{
		bGroup = false;
		Target = "";
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
		float TargetLength;

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
		TargetLength = 3000.0f;
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
		UClass* Class;

	UPROPERTY()
		FTransform Transform;

	UPROPERTY()
		int32 dataIndex;

	UPROPERTY()
		TArray<FTimerStruct> SavedTimers;

	FActorSaveData()
	{
		Actor = nullptr;
		Name = "";
		Class = nullptr;
		Transform = FTransform(FQuat::Identity, FVector(0.0f));
		dataIndex = INDEX_NONE;
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

	UPROPERTY()
		FString SaveName;

	UPROPERTY()
		FRandomStream Stream;

	UPROPERTY()
		TArray<FActorSaveData> SavedActors;

	UPROPERTY()
		TArray<FAIData> AIData;

	UPROPERTY()
		TArray<FBuildingData> BuildingData;

	UPROPERTY()
		TArray<FResourceData> ResourceData;

	UPROPERTY()
		TArray<FProjectileData> ProjectileData;

	UPROPERTY()
		TArray<FSpawnerData> SpawnerData;

	UPROPERTY()
		FWorldSaveData WorldData;

	UPROPERTY()
		FCameraData CameraData;

	FSave()
	{
		SaveName = "";
	}

	void EmptyData()
	{
		SavedActors.Empty();
		AIData.Empty();
		BuildingData.Empty();
		ResourceData.Empty();
		ProjectileData.Empty();
		SpawnerData.Empty();
		WorldData = FWorldSaveData();
		CameraData = FCameraData();
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

	UPROPERTY()
		TArray<FTileData> Tiles;

	int32 SaveGame(class ACamera* Camera, int32 Index, FString ID);

	void LoadGame(class ACamera* Camera, int32 Index);

private:
	// Saving
	void SaveWorld(FActorSaveData& ActorData, AActor* Actor, int32 Index, TArray<AActor*> PotentialWetActors);

	void SaveResource(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, int32 Index);

	void SaveCamera(FActorSaveData& ActorData, AActor* Actor, int32 Index);
	void SaveCitizenManager(FActorSaveData& ActorData, AActor* Actor, int32 Index);
	void SaveFactions(FActorSaveData& ActorData, AActor* Actor, int32 Index);
	void SaveGamemode(FActorSaveData& ActorData, AActor* Actor, int32 Index);

	void SaveAI(class ACamera* Camera, FActorSaveData& ActorData, FAIData& AIData, AActor* Actor, int32 Index);
	void SaveCitizen(class ACamera* Camera, FActorSaveData& ActorData, FAIData& AIData, AActor* Actor, int32 Index);

	void SaveBuilding(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, int32 Index);

	void SaveProjectile(FActorSaveData& ActorData, AActor* Actor, int32 Index);

	void SaveAISpawner(FActorSaveData& ActorData, AActor* Actor, int32 Index);

	void SaveTimers(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor);

	void SaveComponents(FActorSaveData& ActorData, AActor* Actor, int32 Index);

	// Loading
	void LoadWorld(FWorldSaveData WorldData, AActor* Actor, TArray<FWetnessData>& WetnessData);

	void LoadResource(class ACamera* Camera, FResourceData& ResourceData, AActor* Actor);

	void LoadEggBasket(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor);

	void LoadCamera(FActorSaveData& ActorData, FCameraData& CameraData, AActor* Actor);
	void LoadFactions(FActorSaveData& ActorData, FCameraData& CameraData, AActor* Actor);
	void LoadGamemode(class ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, FGamemodeData& GamemodeData, AActor* Actor);

	void LoadAI(class ACamera* Camera, class ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, FAIData& AIData, FGamemodeData& GamemodeData, AActor* Actor, TMap<FString, FActorSaveData*>& AIToName);
	void LoadCitizen(class ACamera* Camera, FActorSaveData& ActorData, FAIData& AIData, FCameraData& CameraData, AActor* Actor);

	void LoadBuilding(class ACamera* Camera, FActorSaveData& ActorData, FBuildingData& BuildingData, TArray<FConstructionData>& ConstructionData, AActor* Actor, TMap<FString, FActorSaveData*>& AIToName, int32 Index);

	void LoadProjectile(FProjectileData& ProjectileData, AActor* Actor);

	void LoadAISpawner(FSpawnerData& SpawnerData, AActor* Actor);

	void LoadComponents(FActorSaveData& ActorData, AActor* Actor, int32 Index);
	void LoadOverlappingEnemies(class ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, FActorSaveData SavedData, int32 Index);

	void LoadTimers(class ACamera* Camera, int32 Index, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData);

	void InitialiseObjects(class ACamera* Camera, class ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, FActorSaveData SavedData, int32 Index);

	void InitialiseAI(class ACamera* Camera, FActorSaveData& ActorData, FAIData& AIData, FActorSaveData SavedData);
	void InitialiseCitizen(class ACamera* Camera, FActorSaveData& ActorData, FAIData& AIData, FActorSaveData SavedData);

	void InitialiseConstructionManager(class ACamera* Camera, FActorSaveData& ActorData, FActorSaveData SavedData, int32 Index);
	void InitialiseCitizenManager(class ACamera* Camera, FActorSaveData& ActorData, FActorSaveData SavedData, int32 Index);
	void InitialiseFactions(class ACamera* Camera, FActorSaveData& ActorData, FActorSaveData SavedData, int32 Index);
	void InitialiseGamemode(class ACamera* Camera, class ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, FActorSaveData SavedData, int32 Index);

	void InitialiseResources(class ACamera* Camera, FActorSaveData& ActorData, FResourceData& ResourceData, FActorSaveData SavedData);
};
