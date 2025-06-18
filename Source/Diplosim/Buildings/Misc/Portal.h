#pragma once

#include "CoreMinimal.h"
#include "Buildings/Misc/Special/Special.h"
#include "Portal.generated.h"

UCLASS()
class DIPLOSIM_API APortal : public ASpecial
{
	GENERATED_BODY()
	
public:
	APortal();

	virtual void Enter(class ACitizen* Citizen) override;
};
