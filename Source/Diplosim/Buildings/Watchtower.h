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
	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	void HideRangeComponent();

	void SetRange(float Z);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class UAttackComponent* AttackComponent;

	int32 BaseRange;
};
