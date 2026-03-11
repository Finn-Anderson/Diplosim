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

	void SpawnCitizens();

private:
	void GetNavigableInstances(class UNavigationSystemV1* Nav, FNavLocation Origin, UHierarchicalInstancedStaticMeshComponent* HISM, TArray<int32>& Instances);
};
