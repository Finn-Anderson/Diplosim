#pragma once

#include "CoreMinimal.h"
#include "Buildings/Misc/Special/Special.h"
#include "PowerPlant.generated.h"

UCLASS()
class DIPLOSIM_API APowerPlant : public ASpecial
{
	GENERATED_BODY()
	
public:
	APowerPlant();

	virtual void OnBuilt() override;
};
