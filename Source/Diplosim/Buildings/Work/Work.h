#pragma once

#include "CoreMinimal.h"
#include "Buildings/Building.h"
#include "Work.generated.h"

UENUM()
enum class EWorkType : uint8
{
	Freetime,
	Work
};

USTRUCT(BlueprintType)
struct FWorkHoursStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Work")
		class ACitizen* Citizen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		float WagePerHour;

	UPROPERTY(BlueprintReadOnly, Category = "Work")
		TMap<int32, EWorkType> WorkHours;

	FWorkHoursStruct()
	{
		Citizen = nullptr;
	}

	bool operator==(const FWorkHoursStruct& other) const
	{
		return (other.Citizen == Citizen);
	}
};

UCLASS()
class DIPLOSIM_API AWork : public ABuilding
{
	GENERATED_BODY()
	
public:
	AWork();

protected:
	virtual void BeginPlay() override;

public:
	// Cost
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		float DefaultWagePerHour;

	// Citizens
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class UStaticMesh* WorkHat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hours")
		TArray<FWorkHoursStruct> WorkHours;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		bool bCanAttendEvents;

	UPROPERTY()
		bool bEmergency;

	virtual bool AddCitizen(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;

	virtual void Enter(class ACitizen* Citizen) override;

	UFUNCTION()
		virtual void Production(class ACitizen* Citizen);

	void CheckWorkStatus(int32 Hour);

	bool IsWorking(class ACitizen* Citizen, int32 Hour = -1);

	bool IsAtWork(class ACitizen* Citizen);

	// Hours
	int32 GetHoursInADay(class ACitizen* Citizen = nullptr, FWorkHoursStruct* WorkHour = nullptr);

	int32 GetWagePerHour(class ACitizen* Citizen);

	int32 GetWage(class ACitizen* Citizen);

	int32 GetAverageWage();

	FWorkHoursStruct* GetBestWorkHours(class ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		void SetNewWorkHours(int32 Index, FWorkHoursStruct NewWorkHours);

	UFUNCTION(BlueprintCallable)
		void UpdateWagePerHour(int32 Index, int32 NewWagePerHour);

	void SetEmergency(bool bStatus);

	// Resources
	UPROPERTY()
		int32 Boosters;

	// Forcefield
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forcefield")
		int32 ForcefieldRange;

protected:
	bool IsCapacityFull();

private:
	void AddToWorkHours(class ACitizen* Citizen, bool bAdd);
};
