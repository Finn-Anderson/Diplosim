#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "CitizenManager.generated.h"

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

	void CitizenGeneralLoop();

	void CalculateCitizenInteractions();

	void CalculateFighting();

	UPROPERTY()
		class ACamera* Camera;

	FCriticalSection CitizenGeneralLoopLock;

	FCriticalSection GoalInteractionsLock;
	FCriticalSection ConversationInteractionsLock;
	FCriticalSection VandalismInteractionsLock;
	FCriticalSection ReportInteractionsLock;

	FCriticalSection FightLock;

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

	// Conversations
	USoundBase* GetConversationSound(ACitizen* Citizen);

	void StartConversation(FFactionStruct* Faction, class ACitizen* Citizen1, class ACitizen* Citizen2, bool bInterrogation);

	UFUNCTION()
		void Interact(FFactionStruct Faction, class ACitizen* Citizen1, class ACitizen* Citizen2);

	// Police
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

	UFUNCTION(BlueprintCallable)
		int32 GetPrayCost(FString FactionName);

	UFUNCTION(BlueprintCallable)
		void Sacrifice(FString FactionName);

	UFUNCTION()
		void IncrementPray(FFactionStruct Faction, FString Type, int32 Increment);

	void SetPrayTimer(FFactionStruct Faction, FString Type);

	// Religion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		TArray<FReligionStruct> Religions;

	// Personality
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<FPersonality> Personalities;

	TArray<FPersonality*> GetCitizensPersonalities(class ACitizen* Citizen);

	float GetAggressiveness(class ACitizen* Citizen);

	void PersonalityComparison(class ACitizen* Citizen1, class ACitizen* Citizen2, int32& Likeness, float& Citizen1Aggressiveness, float& Citizen2Aggressiveness);

	void PersonalityComparison(class ACitizen* Citizen1, class ACitizen* Citizen2, int32& Likeness);
};