#pragma once

#include "CoreMinimal.h"
#include "Buildings/Building.h"
#include "Portal.generated.h"

UCLASS()
class DIPLOSIM_API APortal : public ABuilding
{
	GENERATED_BODY()
	
public:
	APortal();

	virtual void Enter(class ACitizen* Citizen) override;
};
