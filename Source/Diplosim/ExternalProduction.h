#pragma once

#include "CoreMinimal.h"
#include "Production.h"
#include "ExternalProduction.generated.h"

UCLASS()
class DIPLOSIM_API AExternalProduction : public AProduction
{
	GENERATED_BODY()

public:
	virtual void Production(class ACitizen* Citizen) override;
};
