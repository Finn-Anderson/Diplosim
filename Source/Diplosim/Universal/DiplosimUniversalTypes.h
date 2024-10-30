#pragma once

#include "CoreMinimal.h"
#include "DiplosimUniversalTypes.generated.h"

//
// Religion
//
UENUM(BlueprintType, meta = (ScriptName = "EReligion"))
enum class EReligion : uint8
{
	Atheist,
	Egg,
	Chicken,
	Fox
};

//
// Politics
//
UENUM()
enum class EParty : uint8
{
	Undecided,
	Religious,
	Militarist,
	Industrialist,
	Environmentalist,
	Freedom
};

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
		EParty Party;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TEnumAsByte<ESway> Leaning;

	FPartyStruct()
	{
		Party = EParty::Undecided;
		Leaning = ESway::NaN;
	}

	bool operator==(const FPartyStruct& other) const
	{
		return (other.Party == Party) && (other.Leaning == Leaning);
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
		TArray<TSubclassOf<class ABuilding>> Buildings;

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

class DIPLOSIM_API DiplosimUniversalTypes
{
public:
	DiplosimUniversalTypes();
};
