#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "Clinic.generated.h"

UCLASS()
class DIPLOSIM_API AClinic : public AWork
{
	GENERATED_BODY()

public:
	AClinic();

	virtual void Enter(class ACitizen* Citizen) override;
};
