#pragma once

#include "CoreMinimal.h"
#include "Buildings/Production.h"
#include "Farm.generated.h"

UCLASS()
class DIPLOSIM_API AFarm : public AProduction
{
	GENERATED_BODY()

public:
	AFarm();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TSubclassOf<class AVegetation> Crop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		float TimeLength;

	TArray<class AVegetation*> CropList;

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;

	void ProductionDone(ACitizen* Citizen);
};
