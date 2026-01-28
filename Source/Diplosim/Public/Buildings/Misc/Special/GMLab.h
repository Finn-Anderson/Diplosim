#pragma once

#include "CoreMinimal.h"
#include "Buildings/Misc/Special/Special.h"
#include "GMLab.generated.h"

UCLASS()
class DIPLOSIM_API AGMLab : public ASpecial
{
	GENERATED_BODY()
	
public:
	AGMLab();

	virtual void Production(class ACitizen* Citizen) override;
};
