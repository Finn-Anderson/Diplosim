#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "Builder.generated.h"

UCLASS()
class DIPLOSIM_API ABuilder : public AWork
{
	GENERATED_BODY()
	
public:
	ABuilder();

	virtual bool CheckInstant() override;

	virtual void Enter(class ACitizen* Citizen) override;

	void CheckConstruction(class ACitizen* Citizen);

	class ABuilding* Constructing;
};
