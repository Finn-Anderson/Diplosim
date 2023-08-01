#pragma once

#include "CoreMinimal.h"
#include "Building.h"
#include "Work.generated.h"

UCLASS()
class DIPLOSIM_API AWork : public ABuilding
{
	GENERATED_BODY()
	
public:
	AWork();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		int32 Wage;

	virtual void OnBuilt() override;

	virtual void UpkeepCost() override;

public:
	// Citizens
	FTimerHandle FindTimer;

	virtual void FindCitizens() override;

	virtual void AddCitizen(class ACitizen* Citizen) override;

	virtual void RemoveCitizen(class ACitizen* Citizen) override;
};
