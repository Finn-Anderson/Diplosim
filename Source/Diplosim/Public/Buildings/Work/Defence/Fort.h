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

	int32 GetZ();

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;
};
