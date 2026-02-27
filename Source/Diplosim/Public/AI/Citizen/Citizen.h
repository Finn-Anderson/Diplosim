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

	void CitizenSetup(FFactionStruct* Faction);

	void ClearCitizen();

	void ApplyResearch(FFactionStruct* Faction);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building Component")
		class UBuildingComponent* BuildingComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bio Component")
		class UBioComponent* BioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Happiness Component")
		class UHappinessComponent* HappinessComponent;

	UPROPERTY()
		bool bConversing;

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

	// Politics
	void SetPoliticalLeanings();

	UPROPERTY()
		bool bHasBeenLeader;

	// Religion
	UPROPERTY(BlueprintReadOnly, Category = "Religion")
		FSpiritualStruct Spirituality;

	UPROPERTY()
		bool bWorshipping;

	void SetReligion(FFactionStruct* Faction);

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