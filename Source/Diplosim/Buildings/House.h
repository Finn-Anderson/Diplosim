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

	virtual void OnBuilt() override;

	virtual void UpkeepCost() override;

public:
	// Citizens
	FTimerHandle FindTimer;

	virtual void FindCitizens() override;

	virtual void AddCitizen(class ACitizen* Citizen) override;

	virtual void RemoveCitizen(class ACitizen* Citizen) override;
};
