#pragma once

#include "CoreMinimal.h"
#include "Buildings/Building.h"
#include "Parliament.generated.h"

UCLASS()
class DIPLOSIM_API AParliament : public ABuilding
{
	GENERATED_BODY()
	
public:
	AParliament();

	virtual void OnBuilt() override;
};
