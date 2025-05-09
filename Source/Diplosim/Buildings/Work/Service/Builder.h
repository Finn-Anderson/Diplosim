#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "Builder.generated.h"

UCLASS()
class DIPLOSIM_API ABuilder : public AWork
{
	GENERATED_BODY()
	
public:
	ABuilder();

	virtual bool CheckInstant() override;

	virtual void Enter(class ACitizen* Citizen) override;

	void CheckCosts(class ACitizen* Citizen, class ABuilding* Building);

	void AddBuildPercentage(class ACitizen* Citizen, class ABuilding* Building);

	void StartRepairTimer(class ACitizen* Citizen, class ABuilding* Building);

	void Repair(class ACitizen* Citizen, class ABuilding* Building);

	void Done(class ACitizen* Citizen, class ABuilding* Building);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build Status")
		int32 BuildPercentage;
};
