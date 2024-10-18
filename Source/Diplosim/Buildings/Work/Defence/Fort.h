#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Fort.generated.h"

UCLASS()
class DIPLOSIM_API AFort : public AWall
{
	GENERATED_BODY()

public:
	AFort();

	virtual void Enter(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;

	UPROPERTY()
		TArray<class ACitizen*> CitizensWithBoostedDamage;
};
