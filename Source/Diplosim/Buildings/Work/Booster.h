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

	TArray<ABuilding*> GetAffectedBuildings();

	bool DoesPromoteFavouringValues(class ACitizen* Citizen);

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	UFUNCTION(BlueprintCallable)
		void SetBroadcastType(FString Type);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost")
		TMap<TSubclassOf<ABuilding>, FString> BuildingsToBoost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost")
		int32 Range;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Influence")
		bool bHolyPlace;
};
