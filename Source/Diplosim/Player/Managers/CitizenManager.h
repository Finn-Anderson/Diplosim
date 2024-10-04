#pragma once

#include "CoreMinimal.h"
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
enum class EGrade : uint8
{
	Mild,
	Moderate,
	Severe
};

UENUM()
enum class EAffect : uint8
{
	Movement,
	Health,
	Damage
};

USTRUCT(BlueprintType)
struct FAffectStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		EAffect Affect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		float Amount;

	FAffectStruct()
	{
		Affect = EAffect::Movement;
		Amount = 1.0f;
	}
};

USTRUCT(BlueprintType)
struct FConditionStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		EGrade Grade;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 Spreadability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 DeathLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		TArray<TSubclassOf<class ABuilding>> Buildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		TArray<FAffectStruct> Affects;

	FConditionStruct()
	{
		Name = "";
		Grade = EGrade::Mild;
		Spreadability = 0;
		Level = 0;
		DeathLevel = -1;
	}

	bool operator==(const FConditionStruct& other) const
	{
		return (other.Name == Name);
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buildings")
		TArray<class ACitizen*> Citizens;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Citizens")
		TArray<class ABuilding*> Buildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rent")
		TArray<FRentStruct> RentList;

	UPROPERTY()
		TArray<FTimerStruct> Timers;

	// Work
	void CheckWorkStatus(int32 Hour);
		
	// Disease
	void StartDiseaseTimer();

	void SpawnDisease();

	void Infect(ACitizen* Citizen);

	void Cure(ACitizen* Citizen);

	void Injure(ACitizen* Citizen);

	void UpdateHealthText(ACitizen* Citizen);

	void PickCitizenToHeal(ACitizen* Healer);

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

	void CallMass(TArray<TSubclassOf<class ABuilding>> BuildingList);

	void EndMass(EReligion Belief);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		TArray<FEventStruct> Events;

	// Rebel
	void Overthrow();

	void SetupRebel(class ACitizen* Citizen);

	bool IsRebellion();

	UPROPERTY()
		int32 CooldownTimer;
};
