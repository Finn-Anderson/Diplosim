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
	// Range
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		class UDecalComponent* DecalComponent;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		class USphereComponent* SphereComponent;

	UFUNCTION()
		virtual void OnRadialOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		virtual void OnRadialOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Cost
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		float WagePerHour;

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

	void AddToWorkHours(class ACitizen* Citizen, bool bAdd);

	void CheckWorkStatus(int32 Hour);

	bool IsWorking(class ACitizen* Citizen, int32 Hour = -1);

	bool IsAtWork(class ACitizen* Citizen);

	int32 GetHoursInADay(class ACitizen* Citizen);

	int32 GetWage(class ACitizen* Citizen);

	int32 GetAverageWage();

	UFUNCTION(BlueprintCallable)
		void SetNewWorkHours(int32 Index, FWorkHoursStruct NewWorkHours);

	void SetEmergency(bool bStatus);

	// Resources
	virtual void Production(class ACitizen* Citizen);

	UPROPERTY()
		TArray<class ABooster*> Boosters;

	// Forcefield
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forcefield")
		int32 ForcefieldRange;
};
