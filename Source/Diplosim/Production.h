#pragma once

#include "CoreMinimal.h"
#include "Work.h"
#include "Production.generated.h"

UCLASS()
class DIPLOSIM_API AProduction : public AWork
{
	GENERATED_BODY()

public:
	AProduction();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Resource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Storage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 StorageCap;

	FTimerHandle ProdTimer;

	virtual void Production(class ACitizen* Citizen);

	void Store(int32 Amount, class ACitizen* Citizen);
};
