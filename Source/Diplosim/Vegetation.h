#pragma once

#include "CoreMinimal.h"
#include "Resource.h"
#include "Vegetation.generated.h"

UCLASS()
class DIPLOSIM_API AVegetation : public AResource
{
	GENERATED_BODY()
	
public:
	AVegetation();

public:
	virtual void YieldStatus();

	void Grow();
};
