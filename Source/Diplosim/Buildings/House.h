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

	virtual void UpkeepCost() override;

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;


	// Citizens
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
		TArray<TSubclassOf<AResource>> Food;

	FTimerHandle FindTimer;

	virtual void FindCitizens() override;

	virtual void AddCitizen(class ACitizen* Citizen) override;

	virtual void RemoveCitizen(class ACitizen* Citizen) override;
};
