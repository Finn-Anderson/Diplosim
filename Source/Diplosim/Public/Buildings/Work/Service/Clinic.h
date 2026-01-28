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

	virtual bool AddCitizen(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;
};
