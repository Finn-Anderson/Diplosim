#pragma once

#include "CoreMinimal.h"
#include "Tile.h"
#include "Ground.generated.h"

UCLASS()
class DIPLOSIM_API AGround : public ATile
{
	GENERATED_BODY()

public:
	AGround();

public:
	int32 Fertility;

	void SetFertility(int32 Mean);

	int32 GetFertility();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TSubclassOf<class AVegetation> Tree;

	UPROPERTY()
		TArray<class AVegetation*> Trees;

	FTimerHandle TreeTimer;

	void GenerateTree();

	void DeleteTrees();
};
