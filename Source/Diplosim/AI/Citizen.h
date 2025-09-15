#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Player/Managers/ConquestManager.h"
#include "AI/AI.h"
#include "Citizen.generated.h"

USTRUCT()
struct FCarryStruct
{
	GENERATED_USTRUCT_BODY()

	class AResource* Type;

	int32 Amount;

	FCarryStruct()
	{
		Type = nullptr;
		Amount = 0;
	}
};

USTRUCT(BlueprintType)
struct FBuildingStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Building")
		class AHouse* House;

	UPROPERTY(BlueprintReadOnly, Category = "Building")
		class AWork* Employment;

	UPROPERTY(BlueprintReadOnly, Category = "Building")
		class ASchool* School;

	UPROPERTY(BlueprintReadOnly, Category = "Building")
		class AOrphanage* Orphanage;

	UPROPERTY(BlueprintReadOnly, Category = "Building")
		class ABuilding* BuildingAt;

	FVector EnterLocation;

	FBuildingStruct()
	{
		House = nullptr;
		Employment = nullptr;
		School = nullptr;
		Orphanage = nullptr;
		BuildingAt = nullptr;
	}
};

UENUM()
enum class ESex : uint8
{
	NaN,
	Male,
	Female
};

USTRUCT(BlueprintType)
struct FBioStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TWeakObjectPtr<class ACitizen> Mother;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TWeakObjectPtr<class ACitizen> Father;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TWeakObjectPtr<class ACitizen> Partner;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		int32 HoursTogetherWithPartner;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		bool bMarried;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		ESex Sex;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		int32 Age;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		int32 EducationLevel;

	UPROPERTY()
		int32 EducationProgress;

	UPROPERTY()
		int32 PaidForEducationLevel;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TArray<class ACitizen*> Children;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TArray<class ACitizen*> Siblings;

	UPROPERTY()
		bool bAdopted;

	FBioStruct()
	{
		Mother = nullptr;
		Father = nullptr;
		Partner = nullptr;
		HoursTogetherWithPartner = 0;
		bMarried = false;
		Sex = ESex::NaN;
		Age = 0;
		EducationLevel = 0;
		EducationProgress = 0;
		PaidForEducationLevel = 0;
		Name = "Citizen";
		bAdopted = false;
	}
};

UENUM(BlueprintType)
enum class EAttendStatus : uint8
{
	Neutral,
	Attended,
	Missed
};

USTRUCT(BlueprintType)
struct FSpiritualStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Religion")
		FString Faith;

	FString FathersFaith;

	FString MothersFaith;

	FSpiritualStruct()
	{
		Faith = "Atheist";
		FathersFaith = "Atheist";
		MothersFaith = "Atheist";
	}

	bool operator==(const FSpiritualStruct& other) const
	{
		return (other.Faith == Faith);
	}
};

USTRUCT()
struct FCollidingStruct
{
	GENERATED_USTRUCT_BODY()

	AActor* Actor;

	int32 Instance;

	FCollidingStruct()
	{
		Actor = nullptr;
		Instance = -1;
	}

	bool operator==(const FCollidingStruct& other) const
	{
		return (other.Actor == Actor) && (other.Instance == Instance);
	}
};

USTRUCT(BlueprintType)
struct FHappinessStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Happiness")
		TMap<FString, int32> Modifiers;

	FHappinessStruct()
	{
		ClearValues();
	}

	void ClearValues()
	{
		Modifiers.Empty();
	}

	void SetValue(FString Key, int32 Value)
	{
		Modifiers.Add(Key, Value);
	}

	void RemoveValue(FString Key)
	{
		Modifiers.Remove(Key);
	}
};

UENUM(BlueprintType)
enum class EGeneticsType : uint8
{
	Speed,
	Shell,
	Reach,
	Awareness,
	Productivity,
	Fertility
};

UENUM(BlueprintType)
enum class EGeneticsGrade : uint8
{
	Neutral,
	Bad,
	Good
};

USTRUCT(BlueprintType)
struct FGeneticsStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetics")
		EGeneticsType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetics")
		EGeneticsGrade Grade;

	FGeneticsStruct()
	{
		Type = EGeneticsType::Speed;
		Grade = EGeneticsGrade::Neutral;
	}

	bool operator==(const FGeneticsStruct& other) const
	{
		return (other.Type == Type);
	}
};

USTRUCT(BlueprintType)
struct FWorkHours
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		class AWork* Work;

	UPROPERTY()
		TArray<int32> Hours;

	FWorkHours()
	{
		Work = nullptr;
	}

	bool operator==(const FWorkHours& other) const
	{
		return (other.Work == Work);
	}
};


UCLASS()
class DIPLOSIM_API ACitizen : public AAI
{
	GENERATED_BODY()

public:
	ACitizen();

protected:
	virtual void BeginPlay() override;

public:
	void CitizenSetup(FFactionStruct* Faction);

	void ClearCitizen();

	void ApplyResearch(FFactionStruct* Faction);

	// Find Job, House and Education
	void FindEducation(class ASchool* Education, int32 TimeToCompleteDay);

	void FindJob(class AWork* Job, int32 TimeToCompleteDay);

	void FindHouse(class AHouse* House, int32 TimeToCompleteDay);

	void SetJobHouseEducation(int32 TimeToCompleteDay);

	float GetAcquiredTime(int32 Index);

	void SetAcquiredTime(int32 Index, float Time);

	bool CanFindAnything(int32 TimeToCompleteDay, FFactionStruct* Faction);

	UPROPERTY()
		TArray<float> TimeOfAcquirement;

	UPROPERTY()
		TArray<class ABuilding*> AllocatedBuildings;

	UPROPERTY()
		ACitizen* FoundHouseOccupant;

	// On Hit
	void SetHarvestVisuals(class AResource* Resource);

	// Audio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* AmbientAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		TArray<USoundBase*> Chops;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		TArray<USoundBase*> Mines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
		class TArray<USoundBase*> NormalConversations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
		class TArray<USoundBase*> IneptIdiotConversations;

	UPROPERTY()
		float VoicePitch;

	// Education
	bool CanAffordEducationLevel();

	void PayForEducationLevels();

	// Work
	bool CanWork(class ABuilding* WorkBuilding);

	bool WillWork();

	float GetProductivity();

	void Heal(ACitizen* Citizen);

	int32 GetLeftoverMoney();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money")
		int32 Balance;

	UPROPERTY(BlueprintReadOnly, Category = "Hours")
		int32 IdealHoursWorkedMin;

	UPROPERTY(BlueprintReadOnly, Category = "Hours")
		int32 IdealHoursWorkedMax;

	UPROPERTY(BlueprintReadOnly, Category = "Hours")
		TArray<FWorkHours> HoursWorked;

	// Buildings
	UPROPERTY(BlueprintReadOnly, Category = "Buildings")
		FBuildingStruct Building;

	// Resources
	void StartHarvestTimer(class AResource* Resource);

	void HarvestResource(class AResource* Resource);

	void Carry(class AResource* Resource, int32 Amount, AActor* Location);

	UPROPERTY()
		FCarryStruct Carrying;

	// Food
	void Eat();

	UPROPERTY(BlueprintReadOnly, Category = "Food")
		int32 Hunger;

	// Energy
	void CheckGainOrLoseEnergy();

	void LoseEnergy();

	void GainEnergy();

	UPROPERTY(BlueprintReadOnly, Category = "Energy")
		int32 Energy;

	UPROPERTY(BlueprintReadOnly, Category = "Energy")
		bool bGain;

	// Bio
	void Birthday();

	void SetSex(TArray<ACitizen*> Citizens);

	void SetName();

	void FindPartner();

	void SetPartner(ACitizen* Citizen);

	void IncrementHoursTogetherWithPartner();

	void HaveChild();

	void RemoveFromHouse();

	TArray<ACitizen*> GetLikedFamily(bool bFactorAge);

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		FBioStruct BioStruct;

	UPROPERTY(BlueprintReadOnly, Category = "Age")
		float SpeedBeforeOld;

	UPROPERTY(BlueprintReadOnly, Category = "Age")
		float MaxHealthBeforeOld;

	// Politics
	void SetPoliticalLeanings();

	bool bHasBeenLeader;

	// Religion
	UPROPERTY(BlueprintReadOnly, Category = "Religion")
		FSpiritualStruct Spirituality;

	UPROPERTY()
		bool bWorshipping;

	UPROPERTY(BlueprintReadOnly, Category = "Religion")
		EAttendStatus MassStatus;

	void SetReligion();

	// Status
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
		TArray<FConditionStruct> HealthIssues;

	// Happiness
	UPROPERTY(BlueprintReadOnly, Category = "Happiness")
		FHappinessStruct Happiness;

	UPROPERTY()
		int32 SadTimer;

	UPROPERTY()
		bool bHolliday;

	UPROPERTY(BlueprintReadOnly, Category = "Festival")
		EAttendStatus FestivalStatus;

	UPROPERTY()
		bool bConversing;

	UPROPERTY()
		int32 ConversationHappiness;

	UPROPERTY()
		int32 FamilyDeathHappiness;

	UPROPERTY()
		int32 WitnessedDeathHappiness;

	void SetAttendStatus(EAttendStatus Status, bool bMass);

	void SetHolliday(bool bStatus);

	void SetDecayHappiness(int32* HappinessToDecay, int32 Amount, int32 Min = -24, int32 Max = 24);

	void DecayHappiness();

	UFUNCTION(BlueprintCallable)
		int32 GetHappiness();

	void SetHappiness();

	// Genetics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetics")
		TArray<FGeneticsStruct> Genetics;

	UPROPERTY()
		bool bGlasses;

	void GenerateGenetics(FFactionStruct* Faction);

	void ApplyGeneticAffect(FGeneticsStruct Genetic);

	// Personality
	void GivePersonalityTrait(ACitizen* Parent = nullptr);

	void ApplyTraitAffect(TMap<FString, float> Affects);

	// Sleep
	UPROPERTY(BlueprintReadOnly, Category = "Sleep")
		bool bSleep;

	UPROPERTY(BlueprintReadOnly, Category = "Hours")
		TArray<int32> HoursSleptToday;

	UPROPERTY(BlueprintReadOnly, Category = "Hours")
		int32 IdealHoursSlept;

	// Multipliers
	UPROPERTY(BlueprintReadOnly, Category = "Multiplier")
		float ProductivityMultiplier;

	UPROPERTY(BlueprintReadOnly, Category = "Multiplier")
		float Fertility;

	UPROPERTY(BlueprintReadOnly, Category = "Multiplier")
		float ReachMultiplier;

	UPROPERTY(BlueprintReadOnly, Category = "Multiplier")
		float AwarenessMultiplier;

	UPROPERTY(BlueprintReadOnly, Category = "Multiplier")
		float FoodMultiplier;

	UPROPERTY(BlueprintReadOnly, Category = "Multiplier")
		float HungerMultiplier;

	UPROPERTY(BlueprintReadOnly, Category = "Multiplier")
		float EnergyMultiplier;

	void ApplyToMultiplier(FString Affect, float Amount);
};