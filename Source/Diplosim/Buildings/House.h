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

	UPROPERTY()
		TArray<FReligionStruct> Religions;

	UPROPERTY()
		TArray<FPartyStruct> Parties;

	virtual void UpkeepCost() override;

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;


	// Citizens
	virtual bool AddCitizen(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;
};
