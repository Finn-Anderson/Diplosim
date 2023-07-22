#pragma once

#include "CoreMinimal.h"
#include "Production.h"
#include "InternalProduction.generated.h"

UCLASS()
class DIPLOSIM_API AInternalProduction : public AProduction
{
	GENERATED_BODY()
	
public:
	virtual void Production(class ACitizen* Citizen) override;

	void ProductionDone(class ACitizen* Citizen);
};
