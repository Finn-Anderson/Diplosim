#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "Watchtower.generated.h"

UCLASS()
class DIPLOSIM_API AWatchtower : public AWork
{
	GENERATED_BODY()
	
public:
	AWatchtower();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class UAttackComponent* AttackComponent;
};
