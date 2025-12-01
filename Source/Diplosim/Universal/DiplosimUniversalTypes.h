#pragma once

#include "CoreMinimal.h"
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

class DIPLOSIM_API DiplosimUniversalTypes
{
public:
	DiplosimUniversalTypes();
};
