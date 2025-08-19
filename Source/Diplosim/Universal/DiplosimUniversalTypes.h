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

class DIPLOSIM_API DiplosimUniversalTypes
{
public:
	DiplosimUniversalTypes();
};
