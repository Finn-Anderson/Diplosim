#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Player/Managers/ConquestManager.h"
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

UENUM()
enum class ERaidPolicy : uint8
{
	Default,
	Home,
	EggTimer
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

	UPROPERTY()
		class ACamera* Camera;

	// Timers
	void CreateTimer(FString Identifier, AActor* Caller, float Time, FTimerDelegate TimerDelegate, bool Repeat, bool OnGameThread = false);

	FTimerStruct* FindTimer(FString ID, AActor* Actor);

	void RemoveTimer(FString ID, AActor* Actor);

	void ResetTimer(FString ID, AActor* Actor);

	void UpdateTimerLength(FString ID, AActor* Actor, int32 NewTarget);

	UFUNCTION(BlueprintCallable)
		int32 GetElapsedTime(FString ID, AActor* Actor);

	UFUNCTION(BlueprintCallable)
		float GetElapsedPercentage(FString ID, AActor* Actor);

	UFUNCTION(BlueprintCallable)
		bool DoesTimerExist(FString ID, AActor* Actor);

	TDoubleLinkedList<FTimerStruct> Timers;

	FCriticalSection LoopLock;

	FCriticalSection DiseaseSpreadLock;

	FCriticalSection CitizenInteractionsLock;

	FCriticalSection TimerLock;

	FCriticalSection FightLock;

	FCriticalSection CloneLock;

	// House
	UFUNCTION(BlueprintCallable)
		void UpdateRent(FString FactionName, TSubclassOf<class AHouse> HouseType, int32 NewRent);

	// Death
	void ClearCitizen(ACitizen* Citizen);

	// Work
	void CheckWorkStatus(int32 Hour);

	ERaidPolicy GetRaidPolicyStatus(ACitizen* Citizen);

	// Citizen
	void CheckUpkeepCosts();

	void CheckCitizenStatus(int32 Hour);

	void CheckForWeddings(int32 Hour);

	float GetAggressiveness(class ACitizen* Citizen);

	void PersonalityComparison(class ACitizen* Citizen1, class ACitizen* Citizen2, int32& Likeness, float& Citizen1Aggressiveness, float& Citizen2Aggressiveness);

	void PersonalityComparison(class ACitizen* Citizen1, class ACitizen* Citizen2, int32& Likeness);

	void StartConversation(FFactionStruct* Faction, class ACitizen* Citizen1, class ACitizen* Citizen2, bool bInterrogation);

	void Interact(FFactionStruct* Faction, class ACitizen* Citizen1, class ACitizen* Citizen2);

	bool IsCarelessWitness(class ACitizen* Citizen);

	bool IsInAPoliceReport(class ACitizen* Citizen);

	void ChangeReportToMurder(class ACitizen* Citizen);

	void GetCloserToFight(class ACitizen* Citizen, class ACitizen* Target, FVector MidPoint);

	void StopFighting(class ACitizen* Citizen);

	void InterrogateWitnesses(FFactionStruct* Faction, class ACitizen* Officer, class ACitizen* Citizen);

	void GotoClosestWantedMan(class ACitizen* Officer);

	void Arrest(class ACitizen* Officer, class ACitizen* Citizen);

	void SetInNearestJail(FFactionStruct* Faction, class ACitizen* Officer, class ACitizen* Citizen);

	void ItterateThroughSentences();

	void ToggleOfficerLights(class ACitizen* Officer, float Value);

	void CeaseAllInternalFighting(FFactionStruct* Faction);

	int32 GetPoliceReportIndex(class ACitizen* Citizen);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Police")
		TSubclassOf<class AWork> PoliceStationClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Police")
		class UNiagaraSystem* ArrestSystem;
		
	// Disease & Injuries
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
		void CreateEvent(FString FactionName, EEventType Type, TSubclassOf<class ABuilding> Building, class ABuilding* Venue, FString Period, int32 Day, TArray<int32> Hours, bool bRecurring, TArray<ACitizen*> Whitelist, bool bFireFestival = false);

	void ExecuteEvent(FString Period, int32 Day, int32 Hour);

	bool IsAttendingEvent(class ACitizen* Citizen);

	void RemoveFromEvent(class ACitizen* Citizen);

	TMap<FFactionStruct*, TArray<FEventStruct*>> OngoingEvents();

	void GotoEvent(ACitizen* Citizen, FEventStruct* Event);

	void StartEvent(FFactionStruct* Faction, FEventStruct* Event, int32 Hour);

	void EndEvent(FFactionStruct* Faction, FEventStruct* Event, int32 Hour);

	bool UpcomingProtest(FFactionStruct* Faction);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		TArray<FEventStruct> InitEvents;

	// Politics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FPartyStruct> InitParties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FLawStruct> InitLaws;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TSubclassOf<class AResource> Money;

	FPartyStruct* GetMembersParty(ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		FString GetCitizenParty(ACitizen* Citizen);

	void SelectNewLeader(FPartyStruct* Party);

	void StartElectionTimer(FFactionStruct* Faction);

	void Election(FFactionStruct* Faction);

	UFUNCTION(BlueprintCallable)
		void Bribe(class ACitizen* Representative, bool bAgree);

	UFUNCTION(BlueprintCallable)
		void ProposeBill(FString FactionName, FLawStruct Bill);

	void SetElectionBillLeans(FFactionStruct* Faction, FLawStruct* Bill);

	void SetupBill(FFactionStruct* Faction);

	void MotionBill(FFactionStruct* Faction, FLawStruct Bill);

	bool IsInRange(TArray<int32> Range, int32 Value);

	void GetVerdict(FFactionStruct* Faction, class ACitizen* Representative, FLawStruct Bill, bool bCanAbstain, bool bPrediction);

	void TallyVotes(FFactionStruct* Faction, FLawStruct Bill);

	UFUNCTION(BlueprintCallable)
		int32 GetLawValue(FString FactionName, FString BillType);

	UFUNCTION(BlueprintCallable)
		int32 GetCooldownTimer(FString FactionName, FLawStruct Law);

	UFUNCTION(BlueprintCallable)
		FString GetBillPassChance(FString FactionName, FLawStruct Bill);

	// Pensions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pensions")
		int32 IssuePensionHour;

	void IssuePensions(int32 Hour);

	// Fighting
	void Overthrow(FFactionStruct* Faction);

	void SetupRebel(FFactionStruct* Faction, class ACitizen* Citizen);

	bool IsRebellion(FFactionStruct* Faction);

	UPROPERTY()
		TArray<class AAI*> Enemies;

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
		void Sacrifice(FString FactionName);

	// Religion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		TArray<FReligionStruct> Religions;

	// Personality
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<FPersonality> Personalities;

	TArray<FPersonality*> GetCitizensPersonalities(class ACitizen* Citizen);
};