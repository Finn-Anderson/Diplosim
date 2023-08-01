#pragma once

#include "CoreMinimal.h"
#include "Production.h"
#include "ExternalProduction.generated.h"

UCLASS()
class DIPLOSIM_API AExternalProduction : public AProduction
{
	GENERATED_BODY()

public:
	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;
};
