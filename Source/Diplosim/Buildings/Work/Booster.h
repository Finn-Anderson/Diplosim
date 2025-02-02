#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "Booster.generated.h"

UCLASS()
class DIPLOSIM_API ABooster : public AWork
{
	GENERATED_BODY()
	
public:
	ABooster();

	virtual void OnRadialOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	virtual void OnRadialOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost")
		TArray<TSubclassOf<AWork>> WorkplacesToBoost;
};
