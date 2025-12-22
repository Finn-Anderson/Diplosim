#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "AI/AI.h"
#include "Citizen.generated.h"

USTRUCT()
struct FCarryStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		class AResource* Type;

	UPROPERTY()
		int32 Amount;

	FCarryStruct()
	{
		Type = nullptr;
		Amount = 0;
	}
};

UENUM()
enum class ESex : uint8
{
	NaN,
	Male,
	Female
};

UENUM()
enum class ESexuality : uint8
{
	NaN,
	Straight,
	Homosexual,
	Bisexual
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
		ESexuality Sexuality;

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
		Sexuality = ESexuality::NaN;
		Age = 0;
		EducationLevel = 0;
		EducationProgress = 0;
		PaidForEducationLevel = 0;
		Name = "Citizen";
		bAdopted = false;
	}
};

USTRUCT(BlueprintType)
struct FSpiritualStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Religion")
		FString Faith;

	UPROPERTY()
		FString FathersFaith;

	UPROPERTY()
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building Component")
		class UBuildingComponent* BuildingComponent;

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

	// Economy
	bool CanAffordEducationLevel();

	void PayForEducationLevels();

	int32 GetLeftoverMoney();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money")
		int32 Balance;

	// Health
	UFUNCTION()
		void Heal(ACitizen* Citizen);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
		TArray<FConditionStruct> HealthIssues;

	// Resources
	void StartHarvestTimer(class AResource* Resource);

	UFUNCTION()
		void HarvestResource(class AResource* Resource);

	void Carry(class AResource* Resource, int32 Amount, AActor* Location);

	UPROPERTY()
		FCarryStruct Carrying;

	// Food
	UFUNCTION()
		void Eat();

	UPROPERTY(BlueprintReadOnly, Category = "Food")
		int32 Hunger;

	// Energy
	UFUNCTION()
		void CheckGainOrLoseEnergy();

	void LoseEnergy();

	void GainEnergy();

	UPROPERTY(BlueprintReadOnly, Category = "Energy")
		int32 Energy;

	UPROPERTY(BlueprintReadOnly, Category = "Energy")
		bool bGain;

	// Bio
	UFUNCTION()
		void Birthday();

	void SetSex(TArray<ACitizen*> Citizens);

	void SetName();

	void SetSexuality(TArray<ACitizen*> Citizens);

	void FindPartner(FFactionStruct* Faction);

	void SetPartner(ACitizen* Citizen);
	
	void RemoveMarriage();

	void RemovePartner();

	void IncrementHoursTogetherWithPartner();

	void HaveChild();

	TArray<ACitizen*> GetLikedFamily(bool bFactorAge);

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		FBioStruct BioStruct;

	UPROPERTY(BlueprintReadOnly, Category = "Age")
		float SpeedBeforeOld;

	UPROPERTY(BlueprintReadOnly, Category = "Age")
		float MaxHealthBeforeOld;

	// Politics
	void SetPoliticalLeanings();

	UPROPERTY()
		bool bHasBeenLeader;

	// Religion
	UPROPERTY(BlueprintReadOnly, Category = "Religion")
		FSpiritualStruct Spirituality;

	UPROPERTY()
		bool bWorshipping;

	UPROPERTY(BlueprintReadOnly, Category = "Religion")
		EAttendStatus MassStatus;

	void SetReligion(FFactionStruct* Faction);

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

	UPROPERTY()
		int32 DivorceHappiness;

	UFUNCTION()
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

	// Sleep
	UFUNCTION()
		void Snore(bool bCreate = false);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
		TArray<USoundBase*> Snores;

	UPROPERTY(BlueprintReadOnly, Category = "Sleep")
		bool bSleep;

	UPROPERTY(BlueprintReadOnly, Category = "Hours")
		TArray<int32> HoursSleptToday;

	UPROPERTY(BlueprintReadOnly, Category = "Hours")
		int32 IdealHoursSlept;

	// Personality
	void GivePersonalityTrait(ACitizen* Parent = nullptr);

	void ApplyTraitAffect(TMap<FString, float> Affects);

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

	float GetProductivity();
};