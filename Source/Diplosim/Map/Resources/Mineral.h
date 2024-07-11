#pragma once

#include "CoreMinimal.h"
#include "Universal/Resource.h"
#include "Mineral.generated.h"

UCLASS()
class DIPLOSIM_API AMineral : public AResource
{
	GENERATED_BODY()

public:
	AMineral();

public:
	void SetRandomQuantity(int32 Instance);

	virtual void YieldStatus(int32 Instance, int32 Yield) override;
};
