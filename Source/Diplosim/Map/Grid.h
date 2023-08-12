#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid.generated.h"

USTRUCT()
struct FTileStruct
{
	GENERATED_USTRUCT_BODY()

	FString Choice;

	int32 Instance;

	int32 Fertility;

	int32 Coords;

	TArray<class AActor*> Resource;

	FTileStruct()
	{
		Choice = "";
		Instance = 0;
		Fertility = -1;
		Coords = 0;
	}

	int32 GetFertility()
	{
		int32 f = 0;

		if (Fertility == -1) {
			f = FMath::RandRange(0, 3);
		}
		else {
			f = Fertility;
		}

		return f;
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
	// Resource Classes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TSubclassOf<class AMineral> Rock;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TSubclassOf<class AVegetation> Tree;

	// Instance Meshes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMWater;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMGround;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMHill;

	// Dimensions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 Size;

	UPROPERTY()
		class ACamera* Camera;

	// Map
	TArray<FTileStruct> Storage;
	
	void Render();

	void GenerateTile(FString Choice, int32 Mean, int32 x, int32 y);

	void Clear();

	// Resources
	bool Forest;

	void GenerateResource(int32 Pos);

	void GenerateVegetation(int32 Pos, FVector Location);
};
