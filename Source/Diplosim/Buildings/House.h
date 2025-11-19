#pragma once

#include "CoreMinimal.h"
#include "Player/Managers/ConquestManager.h"
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
		int32 Maintenance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House")
		int32 QualityCap;

	int32 GetQuality();

	int32 GetMaintenanceVariance();

	void GetRent(FFactionStruct* Faction, ACitizen* Citizen);

	void GetMaintenance(FFactionStruct* Faction);

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	// Citizens
	virtual bool AddCitizen(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;

	virtual void AddVisitor(class ACitizen* Occupant, class ACitizen* Visitor) override;

	virtual void RemoveVisitor(class ACitizen* Occupant, class ACitizen* Visitor) override;
};
