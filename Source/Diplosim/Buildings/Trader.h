#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "Trader.generated.h"

UCLASS()
class DIPLOSIM_API ATrader : public AWork
{
	GENERATED_BODY()
	
public:
	ATrader();

	virtual void Enter(class ACitizen* Citizen) override;

	void SubmitOrder(class ACitizen* Citizen);
};
