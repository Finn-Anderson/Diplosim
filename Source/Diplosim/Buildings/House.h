#pragma once

#include "CoreMinimal.h"
#include "Building.h"
#include "House.generated.h"

UCLASS()
class DIPLOSIM_API AHouse : public ABuilding
{
	GENERATED_BODY()
	
public:
	AHouse();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		int32 Rent;

	virtual void UpkeepCost(int32 Cost = 0) override;

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;


	// Citizens
	FTimerHandle FindTimer;

	virtual void FindCitizens() override;

	virtual bool AddCitizen(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;
};
