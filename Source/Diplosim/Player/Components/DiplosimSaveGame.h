#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Map/Grid.h"
#include "Universal/Resource.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConstructionManager.h"
#include "Buildings/Work/Work.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "AI/Citizen.h"
#include "DiplosimSaveGame.generated.h"

USTRUCT()
struct FHISMData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY();
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
		bool bRedSun;

	FAtmosphereData()
	{
		WindRotation = FRotator::ZeroRotator;
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

	FCloudData()
	{
		Transform = FTransform();
		Distance = 0.0f;
		bPrecipitation = false;
		bHide = false;
		lightningTimer = 0.0f;
		lightningFrequency = 0.0f;
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
		float bDisasterChance;

	UPROPERTY()
		float Intensity;

	UPROPERTY()
		float Frequency;

	FNaturalDisasterData()
	{
		bDisasterChance = 0.0f;
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
struct FResourceData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FHISMData HISMData;

	UPROPERTY()
		TArray<FWorkerStruct> Workers;

	FResourceData()
	{

	}
};

USTRUCT()
struct FWorldSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FTileData> Tiles;

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

	FWorldSaveData()
	{
		Stream = FRandomStream();
		Size = 0;
		Chunks = 0;
	}
};

USTRUCT()
struct FOccupantData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString OccupantName;

	UPROPERTY()
		TArray<FString> VisitorNames;

	FOccupantData()
	{
		OccupantName = "";
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
		double DeathTime;

	UPROPERTY()
		bool bOperate;

	UPROPERTY()
		TArray<FWorkHoursStruct> WorkHours;

	UPROPERTY()
		TArray<FOccupantData> OccupantsData;

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
		FHappinessStruct Happiness;

	UPROPERTY()
		int32 SadTimer;

	UPROPERTY()
		bool bHolliday;

	UPROPERTY()
		EAttendStatus FestivalStatus;

	UPROPERTY()
		bool bConversing;

	UPROPERTY()
		int32 ConversationHappiness;

	UPROPERTY()
		int32 FamilyDeathHappiness;

	UPROPERTY()
		int32 WitnessedDeathHappiness;

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
		bHolliday = false;
		FestivalStatus = EAttendStatus::Neutral;
		bConversing = false;
		ConversationHappiness = 0;
		FamilyDeathHappiness = 0;
		WitnessedDeathHappiness = 0;
		bSleep = false;
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

	UPROPERTY()
		TArray<FVector> TempPoints;

	UPROPERTY()
		bool bSetPoints;

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
		bSetPoints = false;

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

	FAIData()
	{
		FactionName = "";
		BuildingAtName = "";
		Colour = FLinearColor();
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
		TArray<FString> EnemyNames;

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
		UTexture2D* Flag;

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

	FFactionData()
	{
		Name = "";
		Flag = nullptr;
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
		UClass* Class;

	UPROPERTY()
		FTransform Transform;

	UPROPERTY()
		FWorldSaveData WorldSaveData;

	UPROPERTY()
		FResourceData ResourceData;

	UPROPERTY()
		FBuildingData BuildingData;

	UPROPERTY()
		FAIData AIData;

	UPROPERTY()
		FHealthData HealthData;

	UPROPERTY()
		FAttackData AttackData;

	UPROPERTY()
		FProjectileData ProjectileData;

	UPROPERTY()
		FCameraData CameraData;

	UPROPERTY()
		TArray<FTimerStruct> SavedTimers;

	FActorSaveData()
	{
		Class = nullptr;
		Transform = FTransform(FQuat::Identity, FVector(0.0f));
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
};
