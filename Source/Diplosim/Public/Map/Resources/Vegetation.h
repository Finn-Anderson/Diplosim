#pragma once

#include "CoreMinimal.h"
#include "Universal/Resource.h"
#include "Vegetation.generated.h"

UCLASS()
class DIPLOSIM_API AVegetation : public AResource
{
	GENERATED_BODY()

public:
	AVegetation();

public:
	virtual void YieldStatus(int32 Instance, int32 Yield) override;

	UFUNCTION()
		void Grow();

	bool IsHarvestable(int32 Instance, FVector Scale);

	UFUNCTION()
		void OnFire(int32 Instance);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
		int32 TimeLength;

	UPROPERTY()
		TArray<int32> GrowingInstances;
};
