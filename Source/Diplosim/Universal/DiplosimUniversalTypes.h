#pragma once

#include "CoreMinimal.h"
#include "Resource.h"
#include "DiplosimUniversalTypes.generated.h"

//
// Politics
//
UENUM()
enum ESway : uint8
{
	NaN,
	Moderate,
	Strong,
	Radical
};

USTRUCT(BlueprintType)
struct FPartyStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		FString Party;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FString> Agreeable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Politics")
		TMap<class ACitizen*, TEnumAsByte<ESway>> Members;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Politics")
		class ACitizen* Leader;

	FPartyStruct()
	{
		Party = "";
		Leader = nullptr;
	}

	bool operator==(const FPartyStruct& other) const
	{
		return (other.Party == Party);
	}
};

USTRUCT(BlueprintType)
struct FLeanStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		FString Party;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		FString Personality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<int32> ForRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<int32> AgainstRange;

	FLeanStruct()
	{
		Party = "";
		Personality = "";
	}

	bool operator==(const FLeanStruct& other) const
	{
		return (other.Party == Party && other.Personality == Personality);
	}
};

USTRUCT(BlueprintType)
struct FDescriptionStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		int32 Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		int32 Max;

	FDescriptionStruct()
	{
		Description = "";
		Min = 0;
		Max = 0;
	}
};

USTRUCT(BlueprintType)
struct FLawStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		FString BillType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FDescriptionStruct> Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		FString Warning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		int32 Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FLeanStruct> Lean;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		int32 Cooldown;

	FLawStruct()
	{
		BillType = "";
		Warning = "";
		Value = 0.0f;
		Cooldown = 0;
	}

	int32 GetLeanIndex(FString Party = "", FString Personality = "")
	{
		FLeanStruct lean;
		lean.Party = Party;
		lean.Personality = Personality;

		int32 index = Lean.Find(lean);

		return index;
	}

	bool operator==(const FLawStruct& other) const
	{
		return (other.BillType == BillType);
	}
};

//
// Health
//
UENUM()
enum class EGrade : uint8
{
	Mild,
	Moderate,
	Severe
};

UENUM()
enum class EAffect : uint8
{
	Movement,
	Health,
	Damage
};

USTRUCT(BlueprintType)
struct FAffectStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		EAffect Affect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		float Amount;

	FAffectStruct()
	{
		Affect = EAffect::Movement;
		Amount = 1.0f;
	}
};

USTRUCT(BlueprintType)
struct FConditionStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		EGrade Grade;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 Spreadability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 DeathLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		TArray<FAffectStruct> Affects;

	FConditionStruct()
	{
		Name = "";
		Grade = EGrade::Mild;
		Spreadability = 0;
		Level = 0;
		DeathLevel = -1;
	}

	bool operator==(const FConditionStruct& other) const
	{
		return (other.Name == Name);
	}
};

//
// Animations
//
UENUM(BlueprintType)
enum class EAnim : uint8
{
	Still,
	Move,
	Melee,
	Throw,
	Death
};

USTRUCT(BlueprintType)
struct FAnimStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
		EAnim Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
		FTransform EndTransform;

	UPROPERTY()
		FTransform StartTransform;

	UPROPERTY()
		float Alpha;

	UPROPERTY()
		bool bRepeat;

	UPROPERTY()
		float Speed;

	UPROPERTY()
		bool bPlay;

	FAnimStruct()
	{
		Type = EAnim::Still;
		EndTransform = FTransform();
		StartTransform = FTransform();
		Alpha = 0.0f;
		bRepeat = false;
		Speed = 2.0f;
		bPlay = false;
	}

	bool operator==(const FAnimStruct& other) const
	{
		return (other.Type == Type);
	}
};

//
// Research
//
USTRUCT(BlueprintType)
struct FResearchStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		FString ResearchName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		UTexture2D* Texture;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		int32 Level;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		int32 MaxLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		float AmountResearched;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		int32 Target;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		TMap<FString, float> Modifiers;

	FResearchStruct()
	{
		ResearchName = "";
		Texture = nullptr;
		Level = 0;
		MaxLevel = 20;
		AmountResearched = 0;
		Target = 0;
	}

	bool operator==(const FResearchStruct& other) const
	{
		return (other.ResearchName == ResearchName);
	}
};

//
// Events
//
UENUM()
enum class EEventType : uint8
{
	Mass,
	Festival,
	Holliday,
	Marriage,
	Protest
};

UENUM(BlueprintType)
enum class EAttendStatus : uint8
{
	Neutral,
	Attended,
	Missed
};

template<typename T>
FString EnumToString(T EnumValue)
{
	static_assert(TIsUEnumClass<T>::Value, "'T' template parameter to EnumToString must be a valid UEnum");
	return StaticEnum<T>()->GetNameStringByValue((int64)EnumValue);
};

USTRUCT(BlueprintType)
struct FEventStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		EEventType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		FString Period;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		int32 Day;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		TArray<int32> Hours;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		bool bRecurring;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		bool bFireFestival;

	UPROPERTY()
		bool bStarted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		TSubclassOf<class ABuilding> Building;

	UPROPERTY()
		class ABuilding* Venue;

	UPROPERTY()
		TArray<class ACitizen*> Whitelist;

	UPROPERTY()
		TArray<class ACitizen*> Attendees;

	UPROPERTY()
		FVector Location;

	FEventStruct()
	{
		Type = EEventType::Holliday;
		Period = "";
		Day = 0;
		bRecurring = false;
		bStarted = false;
		bFireFestival = false;
		Building = nullptr;
		Venue = nullptr;
		Location = FVector::Zero();
	}

	bool operator==(const FEventStruct& other) const
	{
		return (other.Type == Type && other.Period == Period && other.Day == Day && other.Hours == Hours);
	}
};

//
// Pray
//
USTRUCT()
struct FPrayStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		int32 Good;

	UPROPERTY()
		int32 Bad;

	FPrayStruct()
	{
		Good = 0;
		Bad = 0;
	}
};

//
// Factions
//
// Politics
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

// Police
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

// Resources
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

// Faction
USTRUCT(BlueprintType)
struct FFactionHappinessStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Happiness")
		FString Owner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Happiness")
		TMap<FString, int32> Modifiers;

	UPROPERTY()
		int32 ProposalTimer;

	FFactionHappinessStruct()
	{
		Owner = "";
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
		TArray<class ABuilding*> Buildings;

	UPROPERTY()
		TArray<class ABuilding*> RuinedBuildings;

	UPROPERTY()
		class ABroch* EggTimer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		TArray<FResearchStruct> ResearchStruct;

	UPROPERTY()
		TArray<int32> ResearchIndices;

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

	UPROPERTY()
		int32 FailedBuild;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Army")
		TArray<FArmyStruct> Armies;

	FFactionStruct()
	{
		Name = "";
		Flag = nullptr;
		FlagColour = FLinearColor();
		WarFatigue = 0;
		PartyInPower = "";
		LargestReligion = "";
		EggTimer = nullptr;
		RebelCooldownTimer = 0;
		FailedBuild = 0;
	}

	bool operator==(const FFactionStruct& other) const
	{
		return (other.Name == Name && other.EggTimer == EggTimer);
	}
};

class DIPLOSIM_API DiplosimUniversalTypes
{
public:
	DiplosimUniversalTypes();
};
