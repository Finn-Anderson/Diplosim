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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class USkeletalMeshComponent* WorkItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class USkeletalMeshComponent* WorkHat;

	virtual void FindCitizens() override;

	virtual bool AddCitizen(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;

	// Resources
	FTimerHandle ProdTimer;

	virtual void Production(class ACitizen* Citizen);
};
