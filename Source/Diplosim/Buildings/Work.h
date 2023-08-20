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

	virtual void UpkeepCost() override;

	// Citizens
	FTimerHandle FindTimer;

	virtual void FindCitizens() override;

	virtual void AddCitizen(class ACitizen* Citizen) override;

	virtual void RemoveCitizen(class ACitizen* Citizen) override;

	// Resources
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Resource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Storage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 StorageCap;

	FTimerHandle ProdTimer;

	virtual void Production(class ACitizen* Citizen);

	void Store(int32 Amount, class ACitizen* Citizen);
};
