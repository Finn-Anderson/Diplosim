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

private:
	virtual void BeginPlay() override;

public:
	virtual void YieldStatus(int32 Instance, int32 Yield) override;

	void Grow();

	bool IsHarvestable(int32 Instance, FVector Scale);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
		int32 TimeLength;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY()
		TArray<int32> GrowingInstances;
};
