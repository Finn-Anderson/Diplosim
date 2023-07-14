#pragma once

#include "CoreMinimal.h"
#include "Building.h"
#include "ExternalProduction.generated.h"

UCLASS()
class DIPLOSIM_API AExternalProduction : public ABuilding
{
	GENERATED_BODY()

public:
	virtual void Production(class ACitizen* Citizen) override;

	void StoreResource(AResource* Resource);
};
