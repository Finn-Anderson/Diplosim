#pragma once

#include "CoreMinimal.h"
#include "Building.h"
#include "Production.generated.h"

UCLASS()
class DIPLOSIM_API AProduction : public ABuilding
{
	GENERATED_BODY()

public:
	AProduction();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		float TimeLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> ActorToGetResource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		FString Produce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Storage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 StorageCap;

	FTimerHandle ProdTimer;

	virtual void Action(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen);

	void Store(int32 Amount, class ACitizen* Citizen);
};
