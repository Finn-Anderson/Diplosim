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

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class UAttackComponent* AttackComponent;

	int32 BaseRange;

	void HideRangeComponent();

	void SetRange(float Z);
};
