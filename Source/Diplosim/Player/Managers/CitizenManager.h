#pragma once

#include "CoreMinimal.h"
#include "Player/Camera.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Player/Managers/ConquestManager.h"
#include "Components/ActorComponent.h"
#include "CitizenManager.generated.h"

USTRUCT()
struct FTimerParameterStruct
{
	GENERATED_USTRUCT_BODY()

	UObject* Object;

	UPROPERTY()
		UClass* ObjectClass;

	UPROPERTY()
		FString ObjectName;

	UPROPERTY()
		FVector Location;

	UPROPERTY()
		TArray<FVector> Locations;

	UPROPERTY()
		FLinearColor Colour;

	UPROPERTY()
		bool bStatus;

	UPROPERTY()
		FFactionStruct Faction;

	UPROPERTY()
		float Value;

	UPROPERTY()
		FString String;

	UPROPERTY()
		FGuid ID;

	UPROPERTY()
		FLawStruct Bill;

	UPROPERTY()
		EAttendStatus AttendStatus;

	UPROPERTY()
		TSubclassOf<AResource> Resource;

	FTimerParameterStruct()
	{
		Object = nullptr;
		ObjectClass = nullptr;
		ObjectName = "";
		Location = FVector::Zero();
		Colour = FLinearColor();
		bStatus = false;
		Value = -1001.23f;
		String = "wadaddwr";
		ID = FGuid();
		Bill = FLawStruct();
		AttendStatus = EAttendStatus::Neutral;
		Resource = nullptr;
	}
};

USTRUCT()
struct FTimerStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString ID;

	UPROPERTY()
		AActor* Actor;

	UPROPERTY()
		float Timer;

	UPROPERTY()
		float Target;

	UPROPERTY()
		FName FuncName;

	UPROPERTY()
		TArray<FTimerParameterStruct> Parameters;

	UPROPERTY()
		bool bRepeat;

	UPROPERTY()
		bool bOnGameThread;

	UPROPERTY()
		bool bPaused;

	UPROPERTY()
		bool bModifying;

	UPROPERTY()
		bool bDone;

	UPROPERTY()
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

		FuncName = "";
		Parameters.Empty();
	}

	void CreateTimer(FString Identifier, AActor* Acta, float Time, FName FunctionName, TArray<FTimerParameterStruct> Params, bool Repeat, bool OnGameThread = false)
	{
		ID = Identifier;
		Actor = Acta;
		Target = Time;
		FuncName = FunctionName;
		Parameters = Params;
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

	void TimerLoop();

	void CitizenGeneralLoop();

	void CalculateDisease();

	void CalculateCitizenInteractions();

	void CalculateFighting();

	UPROPERTY()
		class ACamera* Camera;

	// Timers
	void CreateTimer(FString Identifier, AActor* Actor, float Time, FName FunctionName, TArray<FTimerParameterStruct> Params, bool Repeat, bool OnGameThread = false);

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

	TTuple<UObject*, UFunction*> GetFunction(FTimerStruct* Timer);

	void CallTimerFunction(FTimerStruct* Timer);

	template<typename T>
	T GetParameter(FTimerStruct* Timer, int32 Index)
	{
		if constexpr (std::is_base_of_v<UObject, std::remove_pointer_t<T>>)
			return Cast<std::remove_pointer_t<T>>(Timer->Parameters[Index].Object);
		else if constexpr (std::is_same_v<T, FVector>)
			return Timer->Parameters[Index].Location;
		else if constexpr (std::is_same_v<T, TArray<FVector>>)
			return Timer->Parameters[Index].Locations;
		else if constexpr (std::is_same_v<T, FLinearColor>)
			return Timer->Parameters[Index].Colour;
		else if constexpr (std::is_same_v<T, bool>)
			return Timer->Parameters[Index].bStatus;
		else if constexpr (std::is_same_v<T, FFactionStruct>)
			return Timer->Parameters[Index].Faction;
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, float>)
			return Timer->Parameters[Index].Value;
		else if constexpr (std::is_same_v<T, FString>)
			return Timer->Parameters[Index].String;
		else if constexpr (std::is_same_v<T, FGuid>)
			return Timer->Parameters[Index].ID;
		else if constexpr (std::is_same_v<T, FLawStruct>)
			return Timer->Parameters[Index].Bill;
		else if constexpr (std::is_same_v<T, EAttendStatus>)
			return Timer->Parameters[Index].AttendStatus;
		else if constexpr (std::is_same_v<T, TSubclassOf<AResource>>)
			return Timer->Parameters[Index].Resource;
		else
			static_assert(false, "Not a valid type");
	}

	template<typename T>
	void SetParameter(T Value, TArray<FTimerParameterStruct>& Array)
	{
		FTimerParameterStruct param;

		if constexpr (std::is_base_of_v<UObject, std::remove_pointer_t<T>>)
			param.Object = Value;
		else if constexpr (std::is_same_v<T, FVector>)
			param.Location = Value;
		else if constexpr (std::is_same_v<T, TArray<FVector>>)
			param.Locations = Value;
		else if constexpr (std::is_same_v<T, FLinearColor>)
			param.Colour = Value;
		else if constexpr (std::is_same_v<T, bool>)
			param.bStatus = Value;
		else if constexpr (std::is_same_v<T, FFactionStruct>)
			param.Faction = Value;
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, float>)
			param.Value = Value;
		else if constexpr (std::is_same_v<T, FString> || std::is_same_v<T, const char*>)
			param.String = Value;
		else if constexpr (std::is_same_v<T, FGuid>)
			param.ID = Value;
		else if constexpr (std::is_same_v<T, FLawStruct>)
			param.Bill = Value;
		else if constexpr (std::is_same_v<T, EAttendStatus>)
			param.AttendStatus = Value;
		else if constexpr (std::is_same_v<T, TSubclassOf<AResource>>)
			param.Resource;
		else
			static_assert(false, "Not a valid type");

		Array.Add(param);
	}

	TDoubleLinkedList<FTimerStruct> Timers;

	FCriticalSection TimerLoopLock;

	FCriticalSection CitizenGeneralLoopLock;

	FCriticalSection DiseaseSpreadLock;

	FCriticalSection GoalInteractionsLock;
	FCriticalSection ConversationInteractionsLock;
	FCriticalSection VandalismInteractionsLock;
	FCriticalSection ReportInteractionsLock;

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

	USoundBase* GetConversationSound(ACitizen* Citizen);

	void StartConversation(FFactionStruct* Faction, class ACitizen* Citizen1, class ACitizen* Citizen2, bool bInterrogation);

	UFUNCTION()
		void Interact(FFactionStruct Faction, class ACitizen* Citizen1, class ACitizen* Citizen2);

	bool IsCarelessWitness(class ACitizen* Citizen);

	void CreatePoliceReport(FFactionStruct* Faction, class ACitizen* Witness, class ACitizen* Accused, EReportType ReportType, int32& Index);

	bool IsInAPoliceReport(class ACitizen* Citizen, FFactionStruct* Faction);

	void ChangeReportToMurder(class ACitizen* Citizen);

	void GetCloserToFight(class ACitizen* Citizen, class ACitizen* Target, FVector MidPoint);

	void StopFighting(class ACitizen* Citizen);

	UFUNCTION()
		void InterrogateWitnesses(FFactionStruct Faction, class ACitizen* Officer, class ACitizen* Citizen);

	void GotoClosestWantedMan(class ACitizen* Officer);

	void Arrest(class ACitizen* Officer, class ACitizen* Citizen);

	UFUNCTION()
		void SetInNearestJail(FFactionStruct Faction, class ACitizen* Officer, class ACitizen* Citizen);

	void ItterateThroughSentences();

	void CeaseAllInternalFighting(FFactionStruct* Faction);

	int32 GetPoliceReportIndex(class ACitizen* Citizen);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Police")
		TSubclassOf<class AWork> PoliceStationClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Police")
		class UNiagaraSystem* ArrestSystem;
		
	// Disease & Injuries
	void StartDiseaseTimer();

	UFUNCTION()
		void SpawnDisease();

	void Infect(class ACitizen* Citizen);

	void Cure(class ACitizen* Citizen);

	void Injure(class ACitizen* Citizen, int32 Odds);

	void UpdateHealthText(class ACitizen* Citizen);

	TArray<ACitizen*> GetAvailableHealers(FFactionStruct* Faction, TArray<ACitizen*>& Ill, ACitizen* Target);

	void PairCitizenToHealer(FFactionStruct* Faction, ACitizen* Healer = nullptr);

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

	// Events
	UFUNCTION(BlueprintCallable)
		void CreateEvent(FString FactionName, EEventType Type, TSubclassOf<class ABuilding> Building, class ABuilding* Venue, FString Period, int32 Day, TArray<int32> Hours, bool bRecurring, TArray<ACitizen*> Whitelist, bool bFireFestival = false);

	void ExecuteEvent(FString Period, int32 Day, int32 Hour);

	UFUNCTION(BlueprintCallable)
		bool IsAttendingEvent(class ACitizen* Citizen);

	void RemoveFromEvent(class ACitizen* Citizen);

	TMap<FFactionStruct*, TArray<FEventStruct*>> OngoingEvents();

	void GotoEvent(ACitizen* Citizen, FEventStruct* Event, FFactionStruct* Faction = nullptr);

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

	FPartyStruct* GetMembersParty(ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		FString GetCitizenParty(ACitizen* Citizen);

	void SelectNewLeader(FPartyStruct* Party);

	void StartElectionTimer(FFactionStruct* Faction);

	UFUNCTION()
		void Election(FFactionStruct Faction);

	UFUNCTION(BlueprintCallable)
		void Bribe(class ACitizen* Representative, bool bAgree);

	UFUNCTION(BlueprintCallable)
		void ProposeBill(FString FactionName, FLawStruct Bill);

	void SetElectionBillLeans(FFactionStruct* Faction, FLawStruct* Bill);

	void SetupBill(FFactionStruct* Faction);

	UFUNCTION()
		void MotionBill(FFactionStruct Faction, FLawStruct Bill);

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

	UFUNCTION(BlueprintCallable)
		void Pray(FString FactionName);

	UFUNCTION()
		void IncrementPray(FFactionStruct Faction, FString Type, int32 Increment);

	UFUNCTION(BlueprintCallable)
		int32 GetPrayCost(FString FactionName);

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