#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WindComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UWindComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UWindComponent();

protected:
	virtual void BeginPlay() override;

public:	
	FRotator WindRotation;

	void ChangeWindDirection();
};
