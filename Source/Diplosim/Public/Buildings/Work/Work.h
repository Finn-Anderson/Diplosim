#pragma once

#include "CoreMinimal.h"
#include "Buildings/Building.h"
#include "Work.generated.h"

UCLASS()
class DIPLOSIM_API AWork : public ABuilding
{
	GENERATED_BODY()
	
public:
	AWork();

	// Citizens
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class UStaticMesh* WorkHat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		bool bCanAttendEvents;

	UPROPERTY()
		bool bEmergency;

	virtual bool AddCitizen(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;

	virtual void Enter(class ACitizen* Citizen) override;

	UFUNCTION()
		virtual void Production(class ACitizen* Citizen);

	virtual void CheckWorkStatus(int32 Hour);

	bool IsWorking(class ACitizen* Citizen, int32 Hour = -1);

	UFUNCTION(BlueprintCallable)
		virtual bool IsAtWork(class ACitizen* Citizen);

	// Wage + Hours
	virtual void InitialiseCapacityStruct() override;

	int32 GetHoursInADay(class ACitizen* Citizen = nullptr, FCapacityStruct* CapacityStruct = nullptr);

	int32 GetWage(class ACitizen* Citizen);

	int32 GetAverageWage();

	FCapacityStruct* GetBestWorkHours(class ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		void UpdateWorkHours(int32 Index, int32 Hour, EWorkType WorkType);

	UFUNCTION(BlueprintCallable)
		TMap<int32, EWorkType> GetWorkHours(int32 Index);

	UFUNCTION(BlueprintCallable)
		void ResetWorkHours();

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
