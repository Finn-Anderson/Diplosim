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
		Actor = nullptr;
		Timer = 0;
		Target = 0;
		bRepeat = false;
		bPaused = false;
	}

	void CreateTimer(AActor* Caller, int32 Time, FTimerDelegate TimerDelegate, bool Repeat)
	{
		Actor = Caller;
		Target = Time;
		Delegate = TimerDelegate;
		bRepeat = Repeat;
	}

	bool operator==(const FTimerStruct& other) const
	{
		return (other.Actor == Actor && other.Delegate.GetUObject() == Delegate.GetUObject());
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
		Buildings = {};
		Times = {};
	}
};

USTRUCT(BlueprintType)
struct FRentStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rent")
		TSubclassOf<class AHouse> HouseType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rent")
		int32 Rent;

	FRentStruct()
	{
		HouseType = nullptr;
		Rent = 0;
	}
};

USTRUCT(BlueprintType)
struct FLeaderStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		EParty Party;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Politics")
		class ACitizen* Leader;

	FLeaderStruct()
	{
		Party = EParty::Undecided;
		Leader = nullptr;
	}

	bool operator==(const FLeaderStruct& other) const
	{
		return (other.Party == Party);
	}
};

UENUM()
enum class EBillType : uint8
{
	WorkAge,
	VoteAge,
	RepresentativesNum,
	Abolish
};

USTRUCT(BlueprintType)
struct FBillStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		FString Warning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		float Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		bool bIsLaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<EParty> Agreeing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<EParty> Opposing;

	FBillStruct()
	{
		Description = "";
		Value = 0.0f;
		bIsLaw = false;
	}

	bool operator==(const FBillStruct& other) const
	{
		return (other.bIsLaw == bIsLaw);
	}
};

USTRUCT(BlueprintType)
struct FLawStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		EBillType BillType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FBillStruct> Bills;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		int32 Cooldown;

	FLawStruct()
	{
		BillType = EBillType::WorkAge;
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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UCitizenManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCitizenManager();

protected:
	virtual void BeginPlay() override;

public:	
	void Loop();

	void StartTimers();

	void RemoveTimer(AActor* Actor, FTimerDelegate TimerDelegate);

	void ResetTimer(AActor* Actor, FTimerDelegate TimerDelegate);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buildings")
		TArray<class ACitizen*> Citizens;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Citizens")
		TArray<class ABuilding*> Buildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rent")
		TArray<FRentStruct> RentList;

	UPROPERTY()
		TArray<FTimerStruct> Timers;

	UPROPERTY()
		FVector BrochLocation;

	// Work
	void CheckWorkStatus(int32 Hour);
		
	// Disease
	void StartDiseaseTimer();

	void SpawnDisease();

	void Infect(class ACitizen* Citizen);

	void Cure(class ACitizen* Citizen);

	void Injure(class ACitizen* Citizen);

	void UpdateHealthText(class ACitizen* Citizen);

	void PickCitizenToHeal(class ACitizen* Healer);

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Politics")
		TArray<FLeaderStruct> Leaders;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Politics")
		TArray<class ACitizen*> Representatives;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Politics")
		TArray<int32> BribeValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FLawStruct> Laws;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Politics")
		FVoteStruct Votes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FBillStruct> ProposedBills;

	void SelectNewLeader(EParty Party);

	void StartElectionTimer();

	void Election();

	void Bribe(class ACitizen* Representative, bool bAgree);

	void ProposeBill(FBillStruct Bill);

	void MotionBill(FBillStruct Bill);

	void GetInitialVotes(class ACitizen* Representative, FBillStruct Bill);

	void GetVerdict(class ACitizen* Representative, FBillStruct Bill);

	void TallyVotes(FBillStruct Bill);

	float GetLawValue(EBillType BillType);

	// Rebel
	void Overthrow();

	void SetupRebel(class ACitizen* Citizen);

	bool IsRebellion();

	UPROPERTY()
		int32 CooldownTimer;
};
