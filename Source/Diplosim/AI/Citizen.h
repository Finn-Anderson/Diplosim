#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
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
		class ABuilding* BuildingAt;

	FVector EnterLocation;

	FBuildingStruct()
	{
		House = nullptr;
		Employment = nullptr;
		BuildingAt = nullptr;
		School = nullptr;
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
		Sex = ESex::NaN;
		Age = 0;
		EducationLevel = 0;
		EducationProgress = 0;
		PaidForEducationLevel = 0;
		Name = "Citizen";
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
		EReligion Faith;

	EReligion FathersFaith;

	EReligion MothersFaith;

	FSpiritualStruct()
	{
		Faith = EReligion::Atheist;
		FathersFaith = EReligion::Atheist;
		MothersFaith = EReligion::Atheist;
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

UCLASS()
class DIPLOSIM_API ACitizen : public AAI
{
	GENERATED_BODY()

public:
	ACitizen();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintImplementableEvent)
		void SetPopupImageState(FName Command, FName Type);

	UPROPERTY()
		class ACamera* Camera;

	// Find Job, House and Education
	void FindEducation(class ASchool* Education, int32 TimeToCompleteDay);

	void FindJob(class AWork* Job, int32 TimeToCompleteDay);

	void FindHouse(class AHouse* House, int32 TimeToCompleteDay);

	void SetJobHouseEducation(int32 TimeToCompleteDay);

	float GetAcquiredTime(int32 Index);

	void SetAcquiredTime(int32 Index, float Time);

	bool CanFindAnything(int32 TimeToCompleteDay);

	UPROPERTY()
		TArray<float> TimeOfAcquirement;

	UPROPERTY()
		TArray<ABuilding*> AllocatedBuildings;

	// On Hit
	void SetHarvestVisuals(AResource* Resource);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class UNiagaraComponent* HarvestNiagaraComponent;

	// Audio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* AmbientAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		FTimerHandle AmbientAudioHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		TArray<USoundBase*> Chops;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		TArray<USoundBase*> Mines;

	// Cosmetics
	UFUNCTION(BlueprintCallable)
		void SetTorch(int32 Hour);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class UStaticMeshComponent* HatMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class USkeletalMeshComponent* TorchMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class UNiagaraComponent* TorchNiagaraComponent;

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
		TArray<int32> HoursWorked;

	UPROPERTY()
		FTimerHandle HealTimer;

	UPROPERTY()
		ACitizen* CitizenHealing;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
		TArray<TSubclassOf<class AResource>> Food;

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

	void SetSex();

	void SetName();

	void FindPartner();

	void SetPartner(ACitizen* Citizen);

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

	bool bHasBeenLeader;

	// Religion
	UPROPERTY(BlueprintReadOnly, Category = "Religion")
		FSpiritualStruct Spirituality;

	UPROPERTY()
		bool bWorshipping;

	UPROPERTY(BlueprintReadOnly, Category = "Religion")
		EAttendStatus MassStatus;

	void SetReligion();

	// Disease
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UNiagaraComponent* DiseaseNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UWidgetComponent* PopupComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		TArray<FConditionStruct> HealthIssues;

	// Happiness
	UPROPERTY(BlueprintReadOnly, Category = "Happiness")
		FHappinessStruct Happiness;

	UPROPERTY()
		int32 SadTimer;

	UPROPERTY()
		bool bHolliday;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Happiness")
		class UStaticMesh* RebelHat;

	UPROPERTY()
		float Rebel;

	UPROPERTY(BlueprintReadOnly, Category = "Festival")
		EAttendStatus FestivalStatus;

	void SetAttendStatus(EAttendStatus Status, bool bMass);

	void SetHolliday(bool bStatus);

	UFUNCTION(BlueprintCallable)
		int32 GetHappiness();

	void SetHappiness();

	// Genetics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetics")
		TArray<FGeneticsStruct> Genetics;

	void GenerateGenetics();

	void ApplyGeneticAffect(FGeneticsStruct Genetic);

	// Personality
	void GivePersonalityTrait(ACitizen* Parent = nullptr);

	void ApplyTraitAffect(EPersonality Trait);

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