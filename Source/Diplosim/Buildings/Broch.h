#pragma once

#include "CoreMinimal.h"
#include "Buildings/Building.h"
#include "Broch.generated.h"

UCLASS()
class DIPLOSIM_API ABroch : public ABuilding
{
	GENERATED_BODY()

public:
	ABroch();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizens")
		TSubclassOf<class ACitizen> CitizenClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizens")
		int32 NumToSpawn;

	virtual void FindCitizens() override;
};
