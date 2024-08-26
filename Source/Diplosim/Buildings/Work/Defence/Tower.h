#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Tower.generated.h"

UCLASS()
class DIPLOSIM_API ATower : public AWall
{
	GENERATED_BODY()
	
public:
	ATower();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class UAttackComponent* AttackComponent;

	UPROPERTY()
		FLinearColor Colour;
};
