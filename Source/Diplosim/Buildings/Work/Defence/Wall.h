#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "Wall.generated.h"

UCLASS()
class DIPLOSIM_API AWall : public AWork
{
	GENERATED_BODY()
	
public:
	AWall();

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class AProjectile> BuildingProjectileClass;
};
