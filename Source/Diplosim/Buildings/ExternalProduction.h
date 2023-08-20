#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "ExternalProduction.generated.h"

UCLASS()
class DIPLOSIM_API AExternalProduction : public AWork
{
	GENERATED_BODY()

public:
	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;
};
