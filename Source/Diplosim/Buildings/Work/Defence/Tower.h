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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
		class USphereComponent* RangeComponent;

	UPROPERTY()
		FLinearColor Colour;

	UFUNCTION()
		void OnTrapOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnTrapOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
