#pragma once

#include "CoreMinimal.h"
#include "Resource.h"
#include "Mineral.generated.h"

UCLASS()
class DIPLOSIM_API AMineral : public AResource
{
	GENERATED_BODY()

public:
	AMineral();

public:
	UPROPERTY()
		class AGrid* Grid;

	int32 Quantity;

	virtual void YieldStatus();

	void SetQuantity();
};
