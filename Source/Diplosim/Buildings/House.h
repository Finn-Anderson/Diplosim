#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Building.h"
#include "House.generated.h"

UCLASS()
class DIPLOSIM_API AHouse : public ABuilding
{
	GENERATED_BODY()
	
public:
	AHouse();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House")
		int32 Rent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House")
		int32 BaseRent;

	int32 GetSatisfactionLevel();

	void GetRent(FFactionStruct* Faction, ACitizen* Citizen);

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	// Citizens
	virtual bool AddCitizen(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;

	virtual void AddVisitor(class ACitizen* Occupant, class ACitizen* Visitor) override;

	virtual void RemoveVisitor(class ACitizen* Occupant, class ACitizen* Visitor) override;
};
