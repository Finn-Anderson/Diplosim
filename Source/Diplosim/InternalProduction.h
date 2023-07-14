#pragma once

#include "CoreMinimal.h"
#include "Building.h"
#include "InternalProduction.generated.h"

UCLASS()
class DIPLOSIM_API AInternalProduction : public ABuilding
{
	GENERATED_BODY()
	
public:
	virtual void Production(class ACitizen* Citizen) override;
};
