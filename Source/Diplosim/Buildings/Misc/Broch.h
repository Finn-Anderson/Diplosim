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

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizens")
		TSubclassOf<class ACitizen> CitizenClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizens")
		int32 NumToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		class UDecalComponent* DecalComponent;

	virtual void FindCitizens() override;
};
