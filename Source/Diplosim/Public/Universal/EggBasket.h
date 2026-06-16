#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/Grid.h"
#include "EggBasket.generated.h"

UCLASS()
class DIPLOSIM_API AEggBasket : public AActor
{
	GENERATED_BODY()
	
public:	
	AEggBasket();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* BasketMesh;

	void RedeemReward();

private:
	TSubclassOf<class AResource> PickReward(class ACamera* Camera);
};
