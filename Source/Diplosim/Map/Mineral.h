#pragma once

#include "CoreMinimal.h"
#include "Resource.h"
#include "Mineral.generated.h"

UCLASS()
class DIPLOSIM_API AMineral : public AResource
{
	GENERATED_BODY()

public:
	AMineral();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nav")
		class UBoxComponent* BoxCollisionComp;

	class AGrid* Grid;

	int32 Quantity;

	virtual void YieldStatus();

	void SetQuantity();
};
