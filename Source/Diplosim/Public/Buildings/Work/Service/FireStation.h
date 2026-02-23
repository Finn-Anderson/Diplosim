#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "FireStation.generated.h"

UCLASS()
class DIPLOSIM_API AFireStation : public AWork
{
	GENERATED_BODY()
	
public:
	AFireStation();

	virtual void Enter(class ACitizen* Citizen) override;

	virtual bool IsAtWork(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;

	void PutOutFire(ABuilding* Building, ACitizen* Citizen);
};
