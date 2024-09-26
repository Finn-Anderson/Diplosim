#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "Religion.generated.h"

UCLASS()
class DIPLOSIM_API AReligion : public AWork
{
	GENERATED_BODY()
	
public:
	AReligion();

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;
};
