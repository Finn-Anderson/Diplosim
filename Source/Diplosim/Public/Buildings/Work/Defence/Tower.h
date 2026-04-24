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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class UAttackComponent* AttackComponent;

	virtual void SetRotationMesh(int32 yaw) override;

protected:
	virtual void BeginPlay() override;
};
