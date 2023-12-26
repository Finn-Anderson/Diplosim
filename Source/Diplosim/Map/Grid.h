#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid.generated.h"

USTRUCT()
struct FTileStruct
{
	GENERATED_USTRUCT_BODY()

	int32 Level;

	int32 Instance;

	int32 Fertility;

	int32 X;

	int32 Y;

	FQuat Rotation;

	TArray<class AActor*> Resource;

	FTileStruct() {
		Level = -100;

		Instance = 0;

		Fertility = 0;

		X = 0;

		Y = 0;

		Rotation = FRotator(0.0f, 0.0f, 0.0f).Quaternion();
	}

	bool operator==(const FTileStruct& other) const
	{
		return (other.X == X) && (other.Y == Y);
	}
};

UCLASS()
class DIPLOSIM_API AGrid : public AActor
{
	GENERATED_BODY()
	
public:	
	AGrid();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
	TSubclassOf<class AClouds> CloudsClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
		int32 PercentageGround;

	// Resource Classes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TSubclassOf<class AMineral> Rock;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TSubclassOf<class AVegetation> Tree;

	// Instance Meshes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMWater;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMBeach;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMGround;

	// Dimensions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 Size;

	UPROPERTY()
		class ACamera* Camera;

	// Map
	TArray<FTileStruct> Storage;
	
	void Render();

	FTileStruct GetChosenTile(TArray<FTileStruct> Tiles, int32 TargetLevel);

	void FillGaps(FTileStruct Tile);

	void SetBeaches();

	void GenerateTile(int32 Level, int32 Fertility, int32 x, int32 y, FQuat Rotation);

	void Clear();

	// Resources
	bool Forest;

	void GenerateResource(int32 Pos);

	void SpawnResource(int32 Pos, FVector Location, TSubclassOf<class AResource> ResourceClass);
};
