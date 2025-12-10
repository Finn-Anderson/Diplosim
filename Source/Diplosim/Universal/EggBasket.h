#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/Grid.h"
#include "EggBasket.generated.h"

USTRUCT(BlueprintType)
struct FRewardStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
		TSubclassOf<class AResource> Resource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
		int32 Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
		int32 Max;

	FRewardStruct()
	{
		Resource = nullptr;
		Min = 0;
		Max = 0;
	}
};

UCLASS()
class DIPLOSIM_API AEggBasket : public AActor
{
	GENERATED_BODY()
	
public:	
	AEggBasket();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* BasketMesh;

	void RedeemReward();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards")
		TArray<FRewardStruct> Rewards;

	UPROPERTY()
		class AGrid* Grid;

	FTileStruct* Tile;

private:
	FRewardStruct PickReward(class ACamera* Camera);
};
