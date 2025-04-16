#pragma once

#include "CoreMinimal.h"
#include "Buildings/Building.h"
#include "Broch.generated.h"

UCLASS()
class DIPLOSIM_API ABroch : public ABuilding
{
	GENERATED_BODY()

public:
	ABroch();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizens")
		TSubclassOf<class ACitizen> CitizenClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		class UDecalComponent* DecalComponent;

	TArray<TTuple<int32, int32>> Tiles;

	void SpawnCitizens();

	TArray<FVector> GetSpawnLocations(struct FTileStruct StartingTile, struct FTileStruct Tile, int32 Radius, int32 Count = 0);
};
