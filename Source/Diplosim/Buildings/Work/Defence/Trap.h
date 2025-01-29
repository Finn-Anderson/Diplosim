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

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
		class USphereComponent* RangeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
		float FuseTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
		int32 Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
		bool bExplode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
		TSubclassOf<class UCameraShakeBase> Shake;

	UPROPERTY()
		TArray<AActor*> OverlappingActors;

	UFUNCTION()
		void OnTrapOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnTrapOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ActivateTrap();
};
