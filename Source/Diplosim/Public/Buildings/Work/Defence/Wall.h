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

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class USphereComponent* RangeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class AProjectile> BuildingProjectileClass;

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	void SetRotationMesh(int32 yaw);

	void SetRange();
};
