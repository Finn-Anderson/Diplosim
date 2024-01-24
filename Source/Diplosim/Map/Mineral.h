#pragma once

#include "CoreMinimal.h"
#include "Resource.h"
#include "InteractableInterface.h"
#include "Mineral.generated.h"

UCLASS()
class DIPLOSIM_API AMineral : public AResource, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AMineral();

protected:
	virtual void BeginPlay() override;

public:
	virtual void YieldStatus();
};
