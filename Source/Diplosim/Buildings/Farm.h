#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "Farm.generated.h"

UCLASS()
class DIPLOSIM_API AFarm : public AWork
{
	GENERATED_BODY()

public:
	AFarm();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
		TSubclassOf<class AGrid> Grid;

	class AVegetation* Crop;

	virtual void Enter(class ACitizen* Citizen) override;

	void ProductionDone();
};
