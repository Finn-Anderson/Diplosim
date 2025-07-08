#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "CitizenManager.generated.h"

struct FTimerStruct
{
	FString ID;

	AActor* Actor;

	float Timer;

	float Target;

	FTimerDelegate Delegate;

	bool bRepeat;

	bool bOnGameThread;

	bool bPaused;

	bool bModifying;

	bool bDone;

	double LastUpdateTime;

	FTimerStruct()
	{
		ID = "";
		Actor = nullptr;
		Timer = 0;
		Target = 0;
		bRepeat = false;
		bOnGameThread = false;
		bPaused = false;
		bModifying = false;
		bDone = false;
		LastUpdateTime = 0.0f;
	}

	void CreateTimer(FString Identifier, AActor* Caller, float Time, FTimerDelegate TimerDelegate, bool Repeat, bool OnGameThread = false)
	{
		ID = Identifier;
		Actor = Caller;
		Target = Time;
		Delegate = TimerDelegate;
		bRepeat = Repeat;
		bOnGameThread = OnGameThread;
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
	Festival,
	Holliday,
	Protest
};

template<typename T>
FString EnumToString(T EnumValue)
{
	static_assert(TIsUEnumClass<T>::Value, "'T' template parameter to EnumToString must be a valid UEnum");
	return StaticEnum<T>()->GetNameStringByValue((int64)EnumValue);
}

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		bool bRecurring;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		bool bFireFestival;

	UPROPERTY()
		bool bStarted;

	FEventTimeStruct()
	{
		Period = "";
		Day = 0;
		StartHour = 0;
		EndHour = 0;
		bRecurring = false;
		bStarted = false;
		bFireFestival = false;
	}

	bool operator==(const FEventTimeStruct& other) const
	{
		return (other.Period == Period && other.Day == Day && other.StartHour == StartHour && other.EndHour == EndHour);
	}
};


USTRUCT(BlueprintType)
struct FEventStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		EEventType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		TArray<FEventTimeStruct> Times;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		TSubclassOf<class ABuilding> Building;

	UPROPERTY()
		TArray<class ACitizen*> Attendees;

	FEventStruct()
	{
		Type = EEventType::Holliday;
		Times = {};
		Building = nullptr;
	}

	bool operator==(const FEventStruct& other) const
	{
		return (other.Type == Type);
	}
};

USTRUCT(BlueprintType)
struct FPartyStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		FString Party;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FString> Agreeable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
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
		FString Faith;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		TArray<FString> Agreeable;

	FReligionStruct()
	{
		Faith = "";
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
		FString Trait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<FString> Likes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<FString> Dislikes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TMap<FString, float> Affects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		float Aggressiveness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<class ACitizen*> Citizens;

	FPersonality()
	{
		Trait = "";
		Aggressiveness = 1.0f;
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
	void ReadJSONFile(FString path);

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Loop();

	// Timers
	void CreateTimer(FString Identifier, AActor* Caller, float Time, FTimerDelegate TimerDelegate, bool Repeat, bool OnGameThread = false);

	FTimerStruct* FindTimer(FString ID, AActor* Actor);

	void RemoveTimer(FString ID, AActor* Actor);

	void ResetTimer(FString ID, AActor* Actor);

	void UpdateTimerLength(FString ID, AActor* Actor, int32 NewTarget);

	UFUNCTION(BlueprintCallable)
		int32 GetElapsedTime(FString ID, AActor* Actor);

	UFUNCTION(BlueprintCallable)
		bool DoesTimerExist(FString ID, AActor* Actor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Citizens")
		TArray<class ACitizen*> Citizens;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buildings")
		TArray<class ABuilding*> Buildings;

	UPROPERTY()
		TArray<class AAI*> AIPendingRemoval;

	TDoubleLinkedList<FTimerStruct> Timers;

	UPROPERTY()
		FVector BrochLocation;

	FCriticalSection TickLock;

	FCriticalSection TimerLock;

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
	UFUNCTION(BlueprintCallable)
		void CreateEvent(EEventType Type, TSubclassOf<class ABuilding> Building, FString Period, int32 Day, int32 StartHour, int32 EndHour, bool bRecurring, bool bFireFestival = false);

	void ExecuteEvent(FString Period, int32 Day, int32 Hour);

	bool IsAttendingEvent(class ACitizen* Citizen);

	TArray<FEventStruct> OngoingEvents();

	FEventTimeStruct GetOngoingEventTimes(FEventStruct event);

	void GotoEvent(ACitizen* Citizen, FEventStruct Event);

	void StartEvent(FEventStruct Event, FEventTimeStruct Time);

	void EndEvent(FEventStruct Event, FEventTimeStruct Time);

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

	UPROPERTY()
		FVoteStruct Predictions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FLawStruct> ProposedBills;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TSubclassOf<class AResource> Money;

	FPartyStruct* GetMembersParty(ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		FString GetCitizenParty(ACitizen* Citizen);

	void SelectNewLeader(FString Party);

	void StartElectionTimer();

	void Election();

	UFUNCTION(BlueprintCallable)
		void Bribe(class ACitizen* Representative, bool bAgree);

	UFUNCTION(BlueprintCallable)
		void ProposeBill(FLawStruct Bill);

	void SetElectionBillLeans(FLawStruct* Bill);

	void SetupBill();

	void MotionBill(FLawStruct Bill);

	bool IsInRange(TArray<int32> Range, int32 Value);

	void GetVerdict(class ACitizen* Representative, FLawStruct Bill, bool bCanAbstain, bool bPrediction);

	void TallyVotes(FLawStruct Bill);

	UFUNCTION(BlueprintCallable)
		int32 GetLawValue(FString BillType);

	UFUNCTION(BlueprintCallable)
		int32 GetCooldownTimer(FLawStruct Law);

	UFUNCTION(BlueprintCallable)
		FString GetBillPassChance(FLawStruct Bill);

	// Pensions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pensions")
		int32 IssuePensionHour;

	void IssuePensions(int32 Hour);

	// Fighting
	void Overthrow();

	void SetupRebel(class ACitizen* Citizen);

	bool IsRebellion();

	UPROPERTY()
		TArray<class AAI*> Enemies;

	UPROPERTY()
		TArray<class AAI*> Clones;

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