#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "CitizenManager.generated.h"

USTRUCT()
struct FTimerStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString ID;

	UPROPERTY()
		AActor* Actor;

	UPROPERTY()
		int32 Timer;

	UPROPERTY()
		int32 Target;

	FTimerDelegate Delegate;

	UPROPERTY()
		bool bRepeat;

	UPROPERTY()
		bool bPaused;

	FTimerStruct()
	{
		ID = "";
		Actor = nullptr;
		Timer = 0;
		Target = 0;
		bRepeat = false;
		bPaused = false;
	}

	void CreateTimer(FString Identifier, AActor* Caller, int32 Time, FTimerDelegate TimerDelegate, bool Repeat)
	{
		ID = Identifier;
		Actor = Caller;
		Target = Time;
		Delegate = TimerDelegate;
		bRepeat = Repeat;
	}

	bool operator==(const FTimerStruct& other) const
	{
		return (other.ID == ID && other.Actor == Actor);
	}
};

UENUM()
enum class EEventType : uint8
{
	Mass,
	Holiday
};

USTRUCT(BlueprintType)
struct FEventTimeStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		FString Period;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		int32 Day;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		int32 StartHour;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		int32 EndHour;

	FEventTimeStruct()
	{
		Period = "";
		Day = 0;
		StartHour = 0;
		EndHour = 0;
	}
};


USTRUCT(BlueprintType)
struct FEventStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		EEventType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		TArray<TSubclassOf<class ABuilding>> Buildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		TArray<FEventTimeStruct> Times;

	FEventStruct()
	{
		Type = EEventType::Holiday;
		Buildings = {};
		Times = {};
	}
};

USTRUCT(BlueprintType)
struct FPartyStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		EParty Party;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<EPersonality> Agreeable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
		TMap<class ACitizen*, TEnumAsByte<ESway>> Members;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Politics")
		class ACitizen* Leader;

	FPartyStruct()
	{
		Party = EParty::Undecided;
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
		EParty Party;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		EPersonality Personality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<int32> ForRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<int32> AgainstRange;

	FLeanStruct()
	{
		Party = EParty::Undecided;
		Personality = EPersonality::Brave;
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

UENUM()
enum class EBillType : uint8
{
	WorkAge,
	VoteAge,
	Representatives,
	RepresentativeType,
	ElectionTimer,
	FoodCost,
	Pension,
	PensionAge,
	EducationCost,
	EducationAge,
	Election,
	Abolish,
};

USTRUCT(BlueprintType)
struct FLawStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		EBillType BillType;

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
		BillType = EBillType::WorkAge;
		Warning = "";
		Value = 0.0f;
		Cooldown = 0;
	}

	int32 GetLeanIndex(EParty Party = EParty::Undecided, EPersonality Personality = EPersonality::Brave)
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
struct FReligionStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		EReligion Faith;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		TArray<EPersonality> Agreeable;

	FReligionStruct()
	{
		Faith = EReligion::Atheist;
	}

	bool operator==(const FReligionStruct& other) const
	{
		return (other.Faith == Faith);
	}
};

USTRUCT()
struct FPrayStruct
{
	GENERATED_USTRUCT_BODY()

	int32 Good;

	int32 Bad;

	FPrayStruct()
	{
		Good = 0;
		Bad = 0;
	}
};

USTRUCT(BlueprintType)
struct FPersonality
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		EPersonality Trait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<EPersonality> Likes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<EPersonality> Dislikes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<class ACitizen*> Citizens;

	FPersonality()
	{
		Trait = EPersonality::Brave;
	}

	bool operator==(const FPersonality& other) const
	{
		return (other.Trait == Trait);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UCitizenManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCitizenManager();

protected:
	virtual void BeginPlay() override;

	void ReadJSONFile(FString path);

public:	
	void Loop();

	// Timers
	void StartTimers();

	FTimerStruct* FindTimer(FString ID, AActor* Actor);

	void RemoveTimer(FString ID, AActor* Actor);

	void ResetTimer(FString ID, AActor* Actor);

	UFUNCTION(BlueprintCallable)
		int32 GetElapsedTime(FString ID, AActor* Actor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buildings")
		TArray<class ACitizen*> Citizens;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Citizens")
		TArray<class ABuilding*> Buildings;

	UPROPERTY()
		TArray<FTimerStruct> Timers;

	UPROPERTY()
		FVector BrochLocation;

	// House
	UFUNCTION(BlueprintCallable)
		void UpdateRent(TSubclassOf<class AHouse> HouseType, int32 NewRent);

	// Death
	void ClearCitizen(ACitizen* Citizen);

	// Work
	void CheckWorkStatus(int32 Hour);

	// Sleep
	void CheckSleepStatus(int32 Hour);
		
	// Disease
	void StartDiseaseTimer();

	void SpawnDisease();

	void Infect(class ACitizen* Citizen);

	void Cure(class ACitizen* Citizen);

	void Injure(class ACitizen* Citizen, int32 Odds);

	void UpdateHealthText(class ACitizen* Citizen);

	void GetClosestHealer(class ACitizen* Citizen);

	void PickCitizenToHeal(class ACitizen* Healer, class ACitizen* Citizen = nullptr);

	UPROPERTY()
		TArray<class ACitizen*> Infectible;

	UPROPERTY()
		TArray<class ACitizen*> Infected;

	UPROPERTY()
		TArray<class ACitizen*> Injured;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		TArray<FConditionStruct> Diseases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		TArray<FConditionStruct> Injuries;

	UPROPERTY()
		TArray<ACitizen*> Healing;

	// Events
	void ExecuteEvent(FString Period, int32 Day, int32 Hour);

	bool IsWorkEvent(class AWork* Work);

	void CallMass(TArray<TSubclassOf<class ABuilding>> BuildingList);

	void EndMass(EReligion Belief);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		TArray<FEventStruct> Events;

	// Politics
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FLawStruct> ProposedBills;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TSubclassOf<class AResource> Money;

	FPartyStruct* GetMembersParty(ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		EParty GetCitizenParty(ACitizen* Citizen);

	void SelectNewLeader(EParty Party);

	void StartElectionTimer();

	void Election();

	UFUNCTION(BlueprintCallable)
		void Bribe(class ACitizen* Representative, bool bAgree);

	UFUNCTION(BlueprintCallable)
		void ProposeBill(FLawStruct Bill);

	void SetupBill();

	void MotionBill(FLawStruct Bill);

	bool IsInRange(TArray<int32> Range, int32 Value);

	void GetVerdict(class ACitizen* Representative, FLawStruct Bill, bool bCanAbstain);

	void TallyVotes(FLawStruct Bill);

	UFUNCTION(BlueprintCallable)
		int32 GetLawValue(EBillType BillType);

	UFUNCTION(BlueprintCallable)
		int32 GetCooldownTimer(FLawStruct Law);

	// Pensions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pensions")
		int32 IssuePensionHour;

	void IssuePensions(int32 Hour);

	// Rebel
	void Overthrow();

	void SetupRebel(class ACitizen* Citizen);

	bool IsRebellion();

	UPROPERTY()
		int32 CooldownTimer;

	// Genetics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice")
		class UNiagaraSystem* SacrificeSystem;

	UPROPERTY()
		FPrayStruct PrayStruct;

	UFUNCTION(BlueprintCallable)
		void Pray();

	void IncrementPray(FString Type, int32 Increment);

	UFUNCTION(BlueprintCallable)
		int32 GetPrayCost();

	UFUNCTION(BlueprintCallable)
		void Sacrifice();

	// Religion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		TArray<FReligionStruct> Religions;

	// Personality
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<FPersonality> Personalities;

	TArray<FPersonality*> GetCitizensPersonalities(class ACitizen* Citizen);
};