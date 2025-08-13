#pragma once

#include "CoreMinimal.h"
#include "Buildings/Building.h"
#include "Trap.generated.h"

UCLASS()
class DIPLOSIM_API ATrap : public ABuilding
{
	GENERATED_BODY()
	
public:
	ATrap();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
		float FuseTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
		int32 Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
		bool bExplode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
		float Range;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
		float ActivateRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
		TSubclassOf<class UCameraShakeBase> Shake;

	void ShouldStartTrapTimer(AActor* Actor);

	void ActivateTrap();
};
