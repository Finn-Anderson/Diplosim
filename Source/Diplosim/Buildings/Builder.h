#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "Builder.generated.h"

UCLASS()
class DIPLOSIM_API ABuilder : public AWork
{
	GENERATED_BODY()
	
public:
	ABuilder();

	virtual bool CheckInstant() override;

	virtual void Enter(class ACitizen* Citizen) override;

	void CarryResources(class ACitizen* Citizen, class ABuilding* Building);

	void StoreMaterials(class ACitizen* Citizen, class ABuilding* Building);

	void CheckCosts(class ACitizen* Citizen, class ABuilding* Building);

	void CheckGatherSites(class ACitizen* Citizen, struct FCostStruct Stock);

	void AddBuildPercentage(class ACitizen* Citizen, ABuilding* Building);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build Status")
		int32 BuildPercentage;

	FTimerHandle ConstructTimer;
};
